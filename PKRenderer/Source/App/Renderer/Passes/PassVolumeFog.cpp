#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderViewSettings.h"
#include "App/Renderer/RenderPipelineBase.h"
#include "PassVolumeFog.h"

namespace PK::App
{
    PassVolumeFog::PassVolumeFog(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE("PassVolumeFog.Ctor");
        PK_LOG_SCOPE_INDENT(local);
        m_computeDensity = assetDatabase->Find<ShaderAsset>("CS_VolumeFog_Density");
        m_computeInject = assetDatabase->Find<ShaderAsset>("CS_VolumeFog_Inject");
        m_computeScatter = assetDatabase->Find<ShaderAsset>("CS_VolumeFog_Scatter");
        m_shaderComposite = assetDatabase->Find<ShaderAsset>("CS_VolumeFog_Composite");
    }

    void PassVolumeFog::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.FogSettings;

        auto fadeShadowsDirect = 1.0f / (settings.ZFar - glm::mix(settings.ZFar, settings.ZNear, settings.FadeShadowsDirect));
        auto fadeShadowsVolumetric = 1.0f / (settings.ZFar - glm::mix(settings.ZFar, settings.ZNear, settings.FadeShadowsVolumetric));
        auto fadeStatic = 1.0f / (settings.ZFar - glm::mix(settings.ZFar, settings.ZNear, settings.FadeStatic));

        view->constants->Set<float>(hash->pk_Fog_Density_NoiseAmount, settings.DensityNoiseAmount);
        view->constants->Set<float>(hash->pk_Fog_Density_NoiseScale, settings.DensityNoiseScale);
        view->constants->Set<float>(hash->pk_Fog_Density_Amount, settings.Density);
        view->constants->Set<float>(hash->pk_Fog_Phase0, settings.Phase0);
        view->constants->Set<float>(hash->pk_Fog_Phase1, settings.Phase1);
        view->constants->Set<float>(hash->pk_Fog_PhaseW, settings.PhaseW);
        view->constants->Set<float4>(hash->pk_Fog_Albedo, float4(settings.Albedo, 0.0f));
        view->constants->Set<float4>(hash->pk_Fog_Absorption, float4(settings.Absorption, 0.0f));
        view->constants->Set<float4>(hash->pk_Fog_WindDirSpeed, float4(settings.WindDirection, settings.WindSpeed));
        view->constants->Set<float4>(hash->pk_Fog_ZParams, float4(Math::GetExponentialZParams01(settings.ZNear, settings.ZFar, settings.ZDistribution), settings.ZFar));
        view->constants->Set<float4>(hash->pk_Fog_FadeParams, float4(fadeShadowsDirect, fadeShadowsVolumetric, fadeStatic, settings.FadeGroundOcclusion));
        view->constants->Set<float4>(hash->pk_Fog_Density_ExpParams0, *reinterpret_cast<float4*>(&settings.Exponential0.Constant));
        view->constants->Set<float4>(hash->pk_Fog_Density_ExpParams1, *reinterpret_cast<float4*>(&settings.Exponential1.Constant));
    }

    void PassVolumeFog::ComputeDensity(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto hash = HashCache::Get();
        auto view = context->views[0];
        auto resources = view->GetResources<ViewResources>();
        auto resolution = view->GetResolution();
        const uint3 volumeResolution = { resolution.x / 8u, resolution.y / 8u, 128u };

        cmd->BeginDebugScope("Fog.InjectionScattering", PK_COLOR_MAGENTA);

        auto hasResized = false;
        {
            TextureDescriptor descriptor{};
            descriptor.type = TextureType::Texture3D;
            descriptor.format = TextureFormat::RGBA16F;
            descriptor.sampler.filterMin = FilterMode::Bilinear;
            descriptor.sampler.filterMag = FilterMode::Bilinear;
            descriptor.sampler.wrap[0] = WrapMode::Clamp;
            descriptor.sampler.wrap[1] = WrapMode::Clamp;
            descriptor.sampler.wrap[2] = WrapMode::Clamp;
            descriptor.resolution = volumeResolution;
            descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
            hasResized |= RHI::ValidateTexture(resources->volumeScatter, descriptor, "Fog.ScatterVolume");

            descriptor.format = TextureFormat::RGB9E5;
            descriptor.formatAlias = TextureFormat::R32UI;
            hasResized |= RHI::ValidateTexture(resources->volumeInject, descriptor, "Fog.InjectVolume");
            hasResized |= RHI::ValidateTexture(resources->volumeInjectPrev, descriptor, "Fog.InjectVolume.Previous");

            descriptor.format = TextureFormat::R16F;
            descriptor.formatAlias = TextureFormat::Invalid;
            descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
            hasResized |= RHI::ValidateTexture(resources->volumeDensity, descriptor, "Fog.DensityVolume");
            hasResized |= RHI::ValidateTexture(resources->volumeDensityPrev, descriptor, "Fog.DensityVolume.Previous");
        }

        RHI::SetImage(hash->pk_Fog_Inject, resources->volumeInjectPrev.get());
        RHI::SetTexture(hash->pk_Fog_InjectRead, resources->volumeInject.get());
        RHI::SetImage(hash->pk_Fog_Density, resources->volumeDensity.get());
        RHI::SetTexture(hash->pk_Fog_DensityRead, resources->volumeDensityPrev.get());
        cmd.Dispatch(m_computeDensity, hasResized ? 1 : 0, volumeResolution);

        RHI::SetImage(hash->pk_Fog_Density, resources->volumeDensityPrev.get());
        RHI::SetTexture(hash->pk_Fog_DensityRead, resources->volumeDensity.get());

        cmd->EndDebugScope();
    }

    void PassVolumeFog::Compute(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto hash = HashCache::Get();
        auto view = context->views[0];
        auto resources = view->GetResources<ViewResources>();
        auto resolution = view->GetResolution();
        const uint3 volumeResolution = { resolution.x / 8u, resolution.y / 8u, 128u };

        cmd->BeginDebugScope("Fog.InjectionScattering", PK_COLOR_MAGENTA);
       
        RHI::SetImage(hash->pk_Fog_Inject, resources->volumeInject.get());
        RHI::SetTexture(hash->pk_Fog_InjectRead, resources->volumeInjectPrev.get());
        cmd.Dispatch(m_computeInject, 0, volumeResolution);
        RHI::SetImage(hash->pk_Fog_Inject, resources->volumeInjectPrev.get());
        RHI::SetTexture(hash->pk_Fog_InjectRead, resources->volumeInject.get());

        RHI::SetImage(hash->pk_Fog_Scatter, resources->volumeScatter.get());
        RHI::SetTexture(hash->pk_Fog_ScatterRead, resources->volumeScatter.get());
        cmd.Dispatch(m_computeScatter, 0, { volumeResolution.x, volumeResolution.y, 1u });

        cmd->EndDebugScope();
    }

    void PassVolumeFog::Render(CommandBufferExt cmd, RHITexture* destination)
    {
        cmd->BeginDebugScope("Fog.Composite", PK_COLOR_MAGENTA);
        RHI::SetImage(HashCache::Get()->pk_Image, destination);
        cmd.Dispatch(m_shaderComposite, 0, destination->GetResolution());
        cmd->EndDebugScope();
    }

}