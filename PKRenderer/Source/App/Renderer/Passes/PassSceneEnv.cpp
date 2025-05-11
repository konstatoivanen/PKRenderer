#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/Math/FunctionsMisc.h"
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
#include "PassSceneEnv.h"

namespace PK::App
{
    PassSceneEnv::PassSceneEnv(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE_FUNC("");

        auto hash = HashCache::Get();
        m_backgroundShader = assetDatabase->Find<ShaderAsset>("VS_SceneEnv_Background");
        m_integrateSHShader = assetDatabase->Find<ShaderAsset>("CS_SceneEnv_IntegrateSH");
        m_integrateIBLShader = assetDatabase->Find<ShaderAsset>("CS_SceneEnv_IntegrateIBL");
        m_integrateISLShader = assetDatabase->Find<ShaderAsset>("CS_SceneEnv_IntegrateISL");
        RHI::SetTexture(hash->pk_SceneEnv, RHI::GetBuiltInResources()->BlackTexture2D.get());
        RHI::SetTexture(hash->pk_SceneEnv_ISL, RHI::GetBuiltInResources()->BlackTexture2D.get());
    
        CVariableRegister::Create<CVariableFuncSimple>("Renderer.SceneEnv.ForceCapture", [this]() 
        {
            m_forceCapture = true;
        });
    }

    void PassSceneEnv::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.EnvBackgroundSettings;
        auto& fogSettings = view->settings.FogSettings;
        auto resources = view->GetResources<ViewResources>();

        auto prevExposure = 0.0f;
        auto prevDensity = 0.0f;
        auto prevFogExpParams0 = PK_FLOAT4_ZERO;
        auto prevFogExpParams1 = PK_FLOAT4_ZERO;
        
        // Fogging affects scene env. check for deltas in settings.
        view->constants->TryGet<float>(hash->pk_SceneEnv_Exposure, prevExposure);
        view->constants->TryGet<float>(hash->pk_Fog_Density_Amount, prevDensity);
        view->constants->TryGet<float4>(hash->pk_Fog_Density_ExpParams0, prevFogExpParams0);
        view->constants->TryGet<float4>(hash->pk_Fog_Density_ExpParams1, prevFogExpParams1);

        resources->captureIsDirty |= m_forceCapture;
        resources->captureIsDirty |= settings.CaptureInterval >= 0 && resources->captureCounter >= settings.CaptureInterval;
        resources->captureIsDirty |= prevExposure != settings.Exposure;
        resources->captureIsDirty |= prevDensity != fogSettings.Density;
        resources->captureIsDirty |= memcmp(&prevFogExpParams0, reinterpret_cast<float4*>(&fogSettings.Exponential0.Constant), sizeof(float4)) != 0;
        resources->captureIsDirty |= memcmp(&prevFogExpParams1, reinterpret_cast<float4*>(&fogSettings.Exponential1.Constant), sizeof(float4)) != 0;

        resources->captureOrigin = settings.CaptureUsesViewOrigin ? view->position : PK_FLOAT3_ZERO;
        resources->captureOrigin += settings.CaptureOffset;
        resources->captureCounter = glm::min(resources->captureCounter + 1, glm::max(0, settings.CaptureInterval));

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
            resources->captureIsDirty = true;
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
            RHI::ValidateTexture(resources->sceneEnvIBL, descriptor, "Scene.Env.Texture");
            RHI::SetTexture(hash->pk_SceneEnv, resources->sceneEnvIBL.get());

            descriptor.levels = 8;
            descriptor.resolution = { 128, 128, 1 };
            RHI::ValidateTexture(resources->sceneEnvISL, descriptor, "Scene.Env.ISL.Texture");
            RHI::SetTexture(hash->pk_SceneEnv_ISL, resources->sceneEnvISL.get());
        }
    }

    void PassSceneEnv::PreCompute(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto view = context->views[0];
        auto resources = view->GetResources<ViewResources>();

        if (resources->captureIsDirty)
        {
            auto hash = HashCache::Get();
            auto resolution = resources->sceneEnvIBL->GetResolution();

            RHI::SetTexture(hash->pk_SceneEnv, resources->sourceTexture);
            RHI::SetTexture(hash->pk_SceneEnv_ISL, resources->sceneEnvISL.get());
            cmd.Dispatch(m_integrateSHShader, 0, { 1u, 1u, 1u });

            RHI::SetImage(hash->pk_Image, resources->sceneEnvISL.get(), 0, 0);
            RHI::SetImage(hash->pk_Image1, resources->sceneEnvISL.get(), 1, 0);
            RHI::SetImage(hash->pk_Image2, resources->sceneEnvISL.get(), 2, 0);
            RHI::SetImage(hash->pk_Image3, resources->sceneEnvISL.get(), 3, 0);
            cmd.Dispatch(m_integrateISLShader, 0, { 128, 128, 1u });

            RHI::SetImage(hash->pk_Image, resources->sceneEnvISL.get(), 4, 0);
            RHI::SetImage(hash->pk_Image1, resources->sceneEnvISL.get(), 5, 0);
            RHI::SetImage(hash->pk_Image2, resources->sceneEnvISL.get(), 6, 0);
            RHI::SetImage(hash->pk_Image3, resources->sceneEnvISL.get(), 7, 0);
            cmd.Dispatch(m_integrateISLShader, 1, { 8u, 8u, 1u });

            RHI::SetImage(hash->pk_Image, resources->sceneEnvIBL.get(), 0, 0);
            RHI::SetImage(hash->pk_Image1, resources->sceneEnvIBL.get(), 1, 0);
            RHI::SetImage(hash->pk_Image2, resources->sceneEnvIBL.get(), 2, 0);
            RHI::SetImage(hash->pk_Image3, resources->sceneEnvIBL.get(), 3, 0);
            RHI::SetImage(hash->pk_Image4, resources->sceneEnvIBL.get(), 4, 0);
            RHI::SetConstant(hash->pk_SceneEnv_Origin, float4(resources->captureOrigin, 0.0f));
            cmd.Dispatch(m_integrateIBLShader, 0, { resolution.x >> 1u, resolution.y >> 1u, 1u });
            RHI::SetTexture(hash->pk_SceneEnv, resources->sceneEnvIBL.get());
            resources->captureIsDirty = false;
            resources->captureCounter = 0;
            m_forceCapture = false;
        }
    }

    void PassSceneEnv::RenderBackground(CommandBufferExt cmd, [[maybe_unused]] RenderPipelineContext* context)
    {
        cmd.Blit(m_backgroundShader);
    }
}
