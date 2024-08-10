#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/TextureAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderViewSettings.h"
#include "PassEnvBackground.h"

namespace PK::App
{
    PassEnvBackground::PassEnvBackground(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE("PassEnvBackground.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        auto hash = HashCache::Get();
        m_backgroundShader = assetDatabase->Find<ShaderAsset>("VS_EnvBackground");
        m_shShader = assetDatabase->Find<ShaderAsset>("CS_EnvSH");
        m_integrateShader = assetDatabase->Find<ShaderAsset>("CS_EnvIntegrate");
        m_shBuffer = RHI::CreateBuffer<float4>(4ull, BufferUsage::DefaultStorage, "Scene.Env.SHBuffer");
        RHI::SetBuffer(hash->pk_SceneEnv_SH, m_shBuffer.get());
        RHI::SetTexture(hash->pk_SceneEnv, RHI::GetBuiltInResources()->BlackTexture2D.get());
    }

    void PassEnvBackground::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.EnvBackgroundSettings;
        auto& fogSettings = view->settings.FogSettings;

        auto prevExposure = 0.0f;
        auto prevDensity = 0.0f;
        auto prevDensityConstant = 0.0f;
        auto prevDensityHeightAmount = 0.0f;
        auto prevDensityHeightExponent = 0.0f;
        auto prevDensityHeightOffset = 0.0f;
        
        // Fogging affects scene env. check for deltas in settings.
        view->constants->TryGet<float>(hash->pk_SceneEnv_Exposure, prevExposure);
        view->constants->TryGet<float>(hash->pk_Fog_Density_Amount, prevDensity);
        view->constants->TryGet<float>(hash->pk_Fog_Density_Sky_Constant, prevDensityConstant);
        view->constants->TryGet<float>(hash->pk_Fog_Density_Sky_HeightAmount, prevDensityHeightAmount);
        view->constants->TryGet<float>(hash->pk_Fog_Density_Sky_HeightExponent, prevDensityHeightExponent);
        view->constants->TryGet<float>(hash->pk_Fog_Density_Sky_HeightOffset, prevDensityHeightOffset);

        m_isDirty |= prevExposure != settings.Exposure;
        m_isDirty |= prevDensity != fogSettings.Density;
        m_isDirty |= prevDensityConstant != fogSettings.DensitySkyConstant;
        m_isDirty |= prevDensityHeightAmount != fogSettings.DensitySkyHeightAmount;
        m_isDirty |= prevDensityHeightExponent != fogSettings.DensitySkyHeightExponent;
        m_isDirty |= prevDensityHeightOffset != fogSettings.DensitySkyHeightOffset;

        view->constants->Set<float>(hash->pk_SceneEnv_Exposure, settings.Exposure);

        auto texture = settings.EnvironmentTextureAsset != nullptr ? 
            settings.EnvironmentTextureAsset->GetRHI() : 
            RHI::GetBuiltInResources()->WhiteTexture2D.get();
        
        if (texture != m_sourceTexture)
        {
            m_isDirty = true;
            m_sourceTexture = texture;
            auto sampler = texture->GetSamplerDescriptor();
            sampler.wrap[0] = WrapMode::Mirror;
            sampler.wrap[1] = WrapMode::Mirror;
            sampler.wrap[2] = WrapMode::Mirror;
            texture->SetSampler(sampler);

            auto descriptor = texture->GetDescriptor();
            descriptor.format = TextureFormat::RGB9E5;
            descriptor.formatAlias = TextureFormat::R32UI;
            descriptor.usage = TextureUsage::DefaultStorage;
            RHI::ValidateTexture(m_backgroundTexture, descriptor, "Scene.Env.Texture");
            RHI::SetTexture(hash->pk_SceneEnv, m_backgroundTexture.get());
        }
    }

    void PassEnvBackground::PreCompute(CommandBufferExt cmd)
    {
        if (m_isDirty)
        {
            auto hash = HashCache::Get();
            auto resolution = m_backgroundTexture->GetResolution();

            // Not maybe as neat having this be based purely on the source texture but fixes feedback loop of fog into sh.
            RHI::SetTexture(hash->pk_SceneEnv, m_sourceTexture);
            cmd.Dispatch(m_shShader, 0, { 1u, 1u, 1u });

            RHI::SetImage(hash->pk_Image, m_backgroundTexture.get(), 0, 0);
            RHI::SetImage(hash->pk_Image1, m_backgroundTexture.get(), 1, 0);
            RHI::SetImage(hash->pk_Image2, m_backgroundTexture.get(), 2, 0);
            RHI::SetImage(hash->pk_Image3, m_backgroundTexture.get(), 3, 0);
            RHI::SetImage(hash->pk_Image4, m_backgroundTexture.get(), 4, 0);
            cmd.Dispatch(m_integrateShader, { resolution.x >> 1u, resolution.y >> 1u, 1u });
            RHI::SetTexture(hash->pk_SceneEnv, m_backgroundTexture.get());

            m_isDirty = false;
        }
    }

    void PassEnvBackground::RenderBackground(CommandBufferExt cmd)
    {
        cmd.Blit(m_backgroundShader);
    }
}
