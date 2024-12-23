#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderViewSettings.h"
#include "PassVolumeFog.h"

namespace PK::App
{
    PassVolumeFog::PassVolumeFog(AssetDatabase* assetDatabase, const uint2& initialResolution)
    {
        PK_LOG_VERBOSE("PassVolumeFog.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        TextureDescriptor descriptor{};
        descriptor.type = TextureType::Texture3D;
        descriptor.format = TextureFormat::RGBA16F;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;
        descriptor.sampler.wrap[0] = WrapMode::Clamp;
        descriptor.sampler.wrap[1] = WrapMode::Clamp;
        descriptor.sampler.wrap[2] = WrapMode::Clamp;
        descriptor.resolution = { initialResolution.x / 8u, initialResolution.y / 8u, 128 };
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        m_volumeScatter = RHI::CreateTexture(descriptor, "Fog.ScatterVolume");

        descriptor.format = TextureFormat::RGB9E5;
        descriptor.formatAlias = TextureFormat::R32UI;
        m_volumeInject = RHI::CreateTexture(descriptor, "Fog.InjectVolume");
        m_volumeInjectPrev = RHI::CreateTexture(descriptor, "Fog.InjectVolume.Previous");

        descriptor.format = TextureFormat::R16F;
        descriptor.formatAlias = TextureFormat::Invalid;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        m_volumeDensity = RHI::CreateTexture(descriptor, "Fog.DensityVolume");
        m_volumeDensityPrev = RHI::CreateTexture(descriptor, "Fog.DensityVolume.Previous");

        m_computeDensity = assetDatabase->Find<ShaderAsset>("CS_VolumeFog_Density");
        m_computeInject = assetDatabase->Find<ShaderAsset>("CS_VolumeFog_Inject");
        m_computeScatter = assetDatabase->Find<ShaderAsset>("CS_VolumeFog_Scatter");
        m_shaderComposite = assetDatabase->Find<ShaderAsset>("CS_VolumeFog_Composite");
    }

    void PassVolumeFog::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.FogSettings;
        view->constants->Set<float4>(hash->pk_Fog_Albedo, float4(settings.Albedo, 1.0f));
        view->constants->Set<float4>(hash->pk_Fog_Absorption, float4(settings.Absorption, 1.0f));
        view->constants->Set<float4>(hash->pk_Fog_WindDirSpeed, float4(settings.WindDirection, settings.WindSpeed));
        view->constants->Set<float>(hash->pk_Fog_Phase0, settings.Phase0);
        view->constants->Set<float>(hash->pk_Fog_Phase1, settings.Phase1);
        view->constants->Set<float>(hash->pk_Fog_PhaseW, settings.PhaseW);
        view->constants->Set<float>(hash->pk_Fog_Density_Constant, settings.DensityConstant);
        view->constants->Set<float>(hash->pk_Fog_Density_HeightExponent, settings.DensityHeightExponent);
        view->constants->Set<float>(hash->pk_Fog_Density_HeightOffset, settings.DensityHeightOffset);
        view->constants->Set<float>(hash->pk_Fog_Density_HeightAmount, settings.DensityHeightAmount);
        view->constants->Set<float>(hash->pk_Fog_Density_NoiseAmount, settings.DensityNoiseAmount);
        view->constants->Set<float>(hash->pk_Fog_Density_NoiseScale, settings.DensityNoiseScale);
        view->constants->Set<float>(hash->pk_Fog_Density_Amount, settings.Density);
        view->constants->Set<float>(hash->pk_Fog_Density_Sky_Constant, settings.DensitySkyConstant);
        view->constants->Set<float>(hash->pk_Fog_Density_Sky_HeightExponent, settings.DensitySkyHeightExponent);
        view->constants->Set<float>(hash->pk_Fog_Density_Sky_HeightOffset, settings.DensitySkyHeightOffset);
        view->constants->Set<float>(hash->pk_Fog_Density_Sky_HeightAmount, settings.DensitySkyHeightAmount);
    }

    void PassVolumeFog::ComputeDensity(CommandBufferExt cmd, const uint3& resolution)
    {
        cmd->BeginDebugScope("Fog.InjectionScattering", PK_COLOR_MAGENTA);

        auto hash = HashCache::Get();
        const uint3 volumeResolution = { resolution.x / 8u, resolution.y / 8u, 128u };

        RHI::ValidateTexture(m_volumeDensity, volumeResolution);
        RHI::ValidateTexture(m_volumeDensityPrev, volumeResolution);
        RHI::ValidateTexture(m_volumeInject, volumeResolution);
        RHI::ValidateTexture(m_volumeInjectPrev, volumeResolution);
        RHI::ValidateTexture(m_volumeScatter, volumeResolution);

        RHI::SetImage(hash->pk_Fog_Inject, m_volumeInjectPrev.get());
        RHI::SetTexture(hash->pk_Fog_InjectRead, m_volumeInject.get());
        RHI::SetImage(hash->pk_Fog_Density, m_volumeDensity.get());
        RHI::SetTexture(hash->pk_Fog_DensityRead, m_volumeDensityPrev.get());
        cmd.Dispatch(m_computeDensity, 0, volumeResolution);
        RHI::SetImage(hash->pk_Fog_Density, m_volumeDensityPrev.get());
        RHI::SetTexture(hash->pk_Fog_DensityRead, m_volumeDensity.get());

        cmd->EndDebugScope();
    }

    void PassVolumeFog::Compute(CommandBufferExt cmd, const uint3& resolution)
    {
        cmd->BeginDebugScope("Fog.InjectionScattering", PK_COLOR_MAGENTA);

        auto hash = HashCache::Get();
        const uint3 volumeResolution = { resolution.x / 8u, resolution.y / 8u, 128u };

        RHI::SetImage(hash->pk_Fog_Inject, m_volumeInject.get());
        RHI::SetTexture(hash->pk_Fog_InjectRead, m_volumeInjectPrev.get());
        cmd.Dispatch(m_computeInject, 0, volumeResolution);
        RHI::SetImage(hash->pk_Fog_Inject, m_volumeInjectPrev.get());
        RHI::SetTexture(hash->pk_Fog_InjectRead, m_volumeInject.get());

        RHI::SetImage(hash->pk_Fog_Scatter, m_volumeScatter.get());
        RHI::SetTexture(hash->pk_Fog_ScatterRead, m_volumeScatter.get());
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