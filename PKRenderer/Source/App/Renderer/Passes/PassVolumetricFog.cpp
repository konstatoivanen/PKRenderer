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
#include "PassVolumetricFog.h"

namespace PK::App
{
    PassVolumetricFog::PassVolumetricFog(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE_FUNC("");
        m_compute = assetDatabase->Find<ShaderAsset>("CS_VolumetricFog").get();
    }

    void PassVolumetricFog::SetViewConstants(RenderView* view)
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

    void PassVolumetricFog::ComputeDensity(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto view = context->views[0];
        auto resources = view->GetResources<ViewResources>();
        const auto hash = HashCache::Get();
        const uint3 resolution = { uint2(view->GetResolution().xy) / 8u, 128u };
        const auto index_cur = (view->timeRender.frameIndex + 0ull) & 0x1ull;
        const auto index_pre = (view->timeRender.frameIndex + 1ull) & 0x1ull;

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
            descriptor.resolution = resolution;
            descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
            hasResized |= RHI::ValidateTexture(resources->volumeScatter, descriptor, "Fog.ScatterVolume");

            descriptor.format = TextureFormat::RGB9E5;
            descriptor.formatAlias = TextureFormat::R32UI;
            hasResized |= RHI::ValidateTexture(resources->volumeInject[0], descriptor, "Fog.InjectVolume0");
            hasResized |= RHI::ValidateTexture(resources->volumeInject[1], descriptor, "Fog.InjectVolume1");

            descriptor.format = TextureFormat::R16F;
            descriptor.formatAlias = TextureFormat::Invalid;
            descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
            hasResized |= RHI::ValidateTexture(resources->volumeDensity[0], descriptor, "Fog.DensityVolume0");
            hasResized |= RHI::ValidateTexture(resources->volumeDensity[1], descriptor, "Fog.DensityVolume1");
        }

        if (hasResized)
        {
            RHI::SetImage(hash->pk_Fog_Inject_Write, resources->volumeInject[index_pre].get());
            RHI::SetImage(hash->pk_Fog_Density_Write, resources->volumeDensity[index_pre].get());
            cmd.Dispatch(m_compute, PASS_CLEAR, resolution);
        }

        RHI::SetImage(hash->pk_Fog_Density_Write, resources->volumeDensity[index_cur].get());
        RHI::SetTexture(hash->pk_Fog_Density_Read, resources->volumeDensity[index_pre].get());
        cmd.Dispatch(m_compute, PASS_DENSITY, resolution);

        cmd->EndDebugScope();
    }

    void PassVolumetricFog::Compute(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto view = context->views[0];
        auto resources = view->GetResources<ViewResources>();
        const auto hash = HashCache::Get();
        const uint3 resolution = { uint2(view->GetResolution().xy) / 8u, 128u };
        const auto index_cur = (view->timeRender.frameIndex + 0ull) & 0x1ull;
        const auto index_pre = (view->timeRender.frameIndex + 1ull) & 0x1ull;

        cmd->BeginDebugScope("Fog.InjectionScattering", PK_COLOR_MAGENTA);
       
        RHI::SetImage(hash->pk_Fog_Inject_Write, resources->volumeInject[index_cur].get());
        RHI::SetTexture(hash->pk_Fog_Inject_Read, resources->volumeInject[index_pre].get());
        cmd.Dispatch(m_compute, PASS_INJECT, resolution);

        RHI::SetTexture(hash->pk_Fog_Inject_Read, resources->volumeInject[index_cur].get());
        RHI::SetImage(hash->pk_Fog_Scatter_Write, resources->volumeScatter.get());
        cmd.Dispatch(m_compute, PASS_INTEGRATE, { resolution.x, resolution.y, 1u });

        RHI::SetTexture(hash->pk_Fog_Scatter_Read, resources->volumeScatter.get());

        cmd->EndDebugScope();
    }

    void PassVolumetricFog::Render(CommandBufferExt cmd, RHITexture* destination)
    {
        cmd->BeginDebugScope("Fog.Composite", PK_COLOR_MAGENTA);
        RHI::SetImage(HashCache::Get()->pk_Image, destination);
        cmd.Dispatch(m_compute, PASS_COMPOSITE, destination->GetResolution());
        cmd->EndDebugScope();
    }
}
