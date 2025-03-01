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
#include "App/Renderer/RenderPipelineBase.h"
#include "PassEnvBackground.h"

namespace PK::App
{
    PassEnvBackground::PassEnvBackground(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE("PassEnvBackground.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        auto hash = HashCache::Get();
        m_backgroundShader = assetDatabase->Find<ShaderAsset>("VS_EnvBackground");
        m_shShader = assetDatabase->Find<ShaderAsset>("CS_EnvIntegrateSH");
        m_integrateSHShader = assetDatabase->Find<ShaderAsset>("CS_EnvIntegrateIBL");
        RHI::SetTexture(hash->pk_SceneEnv, RHI::GetBuiltInResources()->BlackTexture2D.get());
    }

    void PassEnvBackground::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.EnvBackgroundSettings;
        auto& fogSettings = view->settings.FogSettings;
        auto resources = view->GetResources<ViewResources>();

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

        resources->runtimeIsDirty |= prevExposure != settings.Exposure;
        resources->runtimeIsDirty |= prevDensity != fogSettings.Density;
        resources->runtimeIsDirty |= prevDensityConstant != fogSettings.DensitySkyConstant;
        resources->runtimeIsDirty |= prevDensityHeightAmount != fogSettings.DensitySkyHeightAmount;
        resources->runtimeIsDirty |= prevDensityHeightExponent != fogSettings.DensitySkyHeightExponent;
        resources->runtimeIsDirty |= prevDensityHeightOffset != fogSettings.DensitySkyHeightOffset;

        view->constants->Set<float>(hash->pk_SceneEnv_Exposure, settings.Exposure);

        if (RHI::ValidateBuffer<float4>(resources->sceneEnvSHBuffer, 4ull, BufferUsage::DefaultStorage, "Scene.Env.SHBuffer"))
        {
            RHI::SetBuffer(hash->pk_SceneEnv_SH, resources->sceneEnvSHBuffer.get());
        }

        auto texture = settings.EnvironmentTextureAsset != nullptr ? 
            settings.EnvironmentTextureAsset->GetRHI() : 
            RHI::GetBuiltInResources()->WhiteTexture2D.get();
        
        if (texture != resources->sourceTexture)
        {
            resources->runtimeIsDirty = true;
            resources->sourceTexture = texture;
            auto sampler = texture->GetSamplerDescriptor();
            sampler.wrap[0] = WrapMode::Mirror;
            sampler.wrap[1] = WrapMode::Mirror;
            sampler.wrap[2] = WrapMode::Mirror;
            texture->SetSampler(sampler);

            auto descriptor = texture->GetDescriptor();
            descriptor.format = TextureFormat::RGB9E5;
            descriptor.formatAlias = TextureFormat::R32UI;
            descriptor.usage = TextureUsage::DefaultStorage;
            RHI::ValidateTexture(resources->sceneEnvTexture, descriptor, "Scene.Env.Texture");
            RHI::SetTexture(hash->pk_SceneEnv, resources->sceneEnvTexture.get());
        }
    }

    void PassEnvBackground::PreCompute(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto view = context->views[0];
        auto resources = view->GetResources<ViewResources>();

        if (resources->runtimeIsDirty)
        {
            auto hash = HashCache::Get();
            auto resolution = resources->sceneEnvTexture->GetResolution();

            RHI::SetTexture(hash->pk_SceneEnv, resources->sourceTexture);
            cmd.Dispatch(m_shShader, 0, { 1u, 1u, 1u });

            RHI::SetImage(hash->pk_Image, resources->sceneEnvTexture.get(), 0, 0);
            RHI::SetImage(hash->pk_Image1, resources->sceneEnvTexture.get(), 1, 0);
            RHI::SetImage(hash->pk_Image2, resources->sceneEnvTexture.get(), 2, 0);
            RHI::SetImage(hash->pk_Image3, resources->sceneEnvTexture.get(), 3, 0);
            RHI::SetImage(hash->pk_Image4, resources->sceneEnvTexture.get(), 4, 0);
            cmd.Dispatch(m_integrateSHShader, { resolution.x >> 1u, resolution.y >> 1u, 1u });
            RHI::SetTexture(hash->pk_SceneEnv, resources->sceneEnvTexture.get());
            resources->runtimeIsDirty = false;
        }
    }

    void PassEnvBackground::RenderBackground(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        cmd.Blit(m_backgroundShader);
    }
}
