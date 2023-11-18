#include "PrecompiledHeader.h"
#include "PassVolumeFog.h"
#include "Rendering/HashCache.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::Services;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    PassVolumeFog::PassVolumeFog(AssetDatabase* assetDatabase, const ApplicationConfig* config)
    {
        PK_LOG_VERBOSE("Initializing Volumetric Fog");
        PK_LOG_SCOPE_INDENT(local);

        TextureDescriptor descriptor{};
        descriptor.samplerType = SamplerType::Sampler3D;
        descriptor.format = TextureFormat::RGBA16F;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;
        descriptor.sampler.wrap[0] = WrapMode::Clamp;
        descriptor.sampler.wrap[1] = WrapMode::Clamp;
        descriptor.sampler.wrap[2] = WrapMode::Clamp;
        descriptor.resolution = { config->InitialWidth / 8u, config->InitialHeight / 8u, 128 };
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        m_volumeScatter = Texture::Create(descriptor, "Fog.ScatterVolume");

        descriptor.format = TextureFormat::RGB9E5;
        descriptor.formatAlias = TextureFormat::R32UI;
        m_volumeInject = Texture::Create(descriptor, "Fog.InjectVolume");
        m_volumeInjectPrev = Texture::Create(descriptor, "Fog.InjectVolume.Previous");

        descriptor.format = TextureFormat::R16F;
        descriptor.formatAlias = TextureFormat::Invalid;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        m_volumeDensity = Texture::Create(descriptor, "Fog.DensityVolume");
        m_volumeDensityPrev = Texture::Create(descriptor, "Fog.DensityVolume.Previous");

        m_computeDensity = assetDatabase->Find<Shader>("CS_VolumeFogDensity");
        m_computeInject = assetDatabase->Find<Shader>("CS_VolumeFogInject");
        m_computeScatter = assetDatabase->Find<Shader>("CS_VolumeFogScatter");
        m_shaderComposite = assetDatabase->Find<Shader>("CS_VolumeFogComposite");

        auto hash = HashCache::Get();

        m_volumeResources = CreateRef<ConstantBuffer>(BufferLayout(
            {
                { ElementType::Float4, hash->pk_Fog_Albedo },
                { ElementType::Float4, hash->pk_Fog_Absorption },
                { ElementType::Float4, hash->pk_Fog_WindDirSpeed },
                { ElementType::Float,  hash->pk_Fog_Phase0 },
                { ElementType::Float,  hash->pk_Fog_Phase1 },
                { ElementType::Float,  hash->pk_Fog_PhaseW },
                { ElementType::Float,  hash->pk_Fog_Density_Constant },
                { ElementType::Float,  hash->pk_Fog_Density_HeightExponent },
                { ElementType::Float,  hash->pk_Fog_Density_HeightOffset },
                { ElementType::Float,  hash->pk_Fog_Density_HeightAmount },
                { ElementType::Float,  hash->pk_Fog_Density_NoiseAmount },
                { ElementType::Float,  hash->pk_Fog_Density_NoiseScale },
                { ElementType::Float,  hash->pk_Fog_Density_Amount },
                { ElementType::Float,  hash->pk_Fog_Density_Sky_Constant },
                { ElementType::Float,  hash->pk_Fog_Density_Sky_HeightExponent },
                { ElementType::Float,  hash->pk_Fog_Density_Sky_HeightOffset },
                { ElementType::Float,  hash->pk_Fog_Density_Sky_HeightAmount }
            }), "Fog.Parameters");

        OnUpdateParameters(config);
        GraphicsAPI::SetBuffer(hash->pk_Fog_Parameters, m_volumeResources->GetBuffer());
    }

    void PassVolumeFog::ComputeDensity(CommandBuffer* cmd, const Math::uint3& resolution)
    {
        cmd->BeginDebugScope("Fog.InjectionScattering", PK_COLOR_MAGENTA);

        auto hash = HashCache::Get();
        const uint3 volumeResolution = { resolution.x / 8u, resolution.y / 8u, 128u };
        m_volumeDensity->Validate(volumeResolution);
        m_volumeDensityPrev->Validate(volumeResolution);
        m_volumeInject->Validate(volumeResolution);
        m_volumeInjectPrev->Validate(volumeResolution);
        m_volumeScatter->Validate(volumeResolution);

        GraphicsAPI::SetImage(hash->pk_Fog_Inject, m_volumeInjectPrev.get());
        GraphicsAPI::SetTexture(hash->pk_Fog_InjectRead, m_volumeInject.get());
        GraphicsAPI::SetImage(hash->pk_Fog_Density, m_volumeDensity.get());
        GraphicsAPI::SetTexture(hash->pk_Fog_DensityRead, m_volumeDensityPrev.get());
        cmd->Dispatch(m_computeDensity, 0, volumeResolution);
        GraphicsAPI::SetImage(hash->pk_Fog_Density, m_volumeDensityPrev.get());
        GraphicsAPI::SetTexture(hash->pk_Fog_DensityRead, m_volumeDensity.get());

        cmd->EndDebugScope();
    }

    void PassVolumeFog::Compute(CommandBuffer* cmd, const Math::uint3& resolution)
    {
        cmd->BeginDebugScope("Fog.InjectionScattering", PK_COLOR_MAGENTA);

        auto hash = HashCache::Get();
        const uint3 volumeResolution = { resolution.x / 8u, resolution.y / 8u, 128u };

        GraphicsAPI::SetImage(hash->pk_Fog_Inject, m_volumeInject.get());
        GraphicsAPI::SetTexture(hash->pk_Fog_InjectRead, m_volumeInjectPrev.get());
        cmd->Dispatch(m_computeInject, 0, volumeResolution);
        GraphicsAPI::SetImage(hash->pk_Fog_Inject, m_volumeInjectPrev.get());
        GraphicsAPI::SetTexture(hash->pk_Fog_InjectRead, m_volumeInject.get());

        GraphicsAPI::SetImage(hash->pk_Fog_Scatter, m_volumeScatter.get());
        GraphicsAPI::SetTexture(hash->pk_Fog_ScatterRead, m_volumeScatter.get());
        cmd->Dispatch(m_computeScatter, 0, { volumeResolution.x, volumeResolution.y, 1u });

        cmd->EndDebugScope();
    }

    void PassVolumeFog::Render(CommandBuffer* cmd, Texture* destination)
    {
        cmd->BeginDebugScope("Fog.Composite", PK_COLOR_MAGENTA);
        GraphicsAPI::SetImage(HashCache::Get()->pk_Image, destination);
        cmd->Dispatch(m_shaderComposite, 0, destination->GetResolution());
        cmd->EndDebugScope();
    }

    void PassVolumeFog::OnUpdateParameters(const ApplicationConfig* config)
    {
        auto hash = HashCache::Get();
        m_volumeResources->Set<float4>(hash->pk_Fog_Albedo, float4(config->FogAlbedo.value, 1.0f));
        m_volumeResources->Set<float4>(hash->pk_Fog_Absorption, float4(config->FogAbsorption.value, 1.0f));
        m_volumeResources->Set<float4>(hash->pk_Fog_WindDirSpeed, float4(config->FogWindDirection.value, config->FogWindSpeed));
        m_volumeResources->Set<float>(hash->pk_Fog_Phase0, config->FogPhase0);
        m_volumeResources->Set<float>(hash->pk_Fog_Phase1, config->FogPhase1);
        m_volumeResources->Set<float>(hash->pk_Fog_PhaseW, config->FogPhaseW);
        m_volumeResources->Set<float>(hash->pk_Fog_Density_Constant, config->FogDensityConstant);
        m_volumeResources->Set<float>(hash->pk_Fog_Density_HeightExponent, config->FogDensityHeightExponent);
        m_volumeResources->Set<float>(hash->pk_Fog_Density_HeightOffset, config->FogDensityHeightOffset);
        m_volumeResources->Set<float>(hash->pk_Fog_Density_HeightAmount, config->FogDensityHeightAmount);
        m_volumeResources->Set<float>(hash->pk_Fog_Density_NoiseAmount, config->FogDensityNoiseAmount);
        m_volumeResources->Set<float>(hash->pk_Fog_Density_NoiseScale, config->FogDensityNoiseScale);
        m_volumeResources->Set<float>(hash->pk_Fog_Density_Amount, config->FogDensity);
        m_volumeResources->Set<float>(hash->pk_Fog_Density_Sky_Constant, config->FogDensitySkyConstant);
        m_volumeResources->Set<float>(hash->pk_Fog_Density_Sky_HeightExponent, config->FogDensitySkyHeightExponent);
        m_volumeResources->Set<float>(hash->pk_Fog_Density_Sky_HeightOffset, config->FogDensitySkyHeightOffset);
        m_volumeResources->Set<float>(hash->pk_Fog_Density_Sky_HeightAmount, config->FogDensitySkyHeightAmount);
        m_volumeResources->FlushBuffer(QueueType::Transfer);
    }
}