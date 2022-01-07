#include "PrecompiledHeader.h"
#include "PassVolumeFog.h"
#include "Rendering/HashCache.h"

namespace PK::Rendering::Passes
{
    constexpr static const uint3 InjectThreadCount = { 16u, 2u, 16u };
    constexpr static const uint3 ScatterThreadCount = { 32u,2u,1u };
    constexpr static const uint3 VolumeResolution = { 160u, 90u, 128u };

    PassVolumeFog::PassVolumeFog(AssetDatabase* assetDatabase, const ApplicationConfig* config)
    {
        TextureDescriptor descriptor;
        descriptor.samplerType = SamplerType::Sampler3D;
        descriptor.format = TextureFormat::RGBA16F;
        descriptor.sampler.filter = FilterMode::Trilinear;
        descriptor.sampler.wrap[0] = WrapMode::Clamp;
        descriptor.sampler.wrap[1] = WrapMode::Clamp;
        descriptor.sampler.wrap[2] = WrapMode::Clamp;
        descriptor.resolution = VolumeResolution;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;

        m_volumeInject = Texture::Create(descriptor);
        m_volumeScatter = Texture::Create(descriptor);
        m_depthTiles = Buffer::CreateStorage({ {ElementType::Uint, "DEPTHMAX"} }, VolumeResolution.x * VolumeResolution.y);

        m_computeInject = assetDatabase->Find<Shader>("CS_VolumeFogLightDensity");
        m_computeScatter = assetDatabase->Find<Shader>("CS_VolumeFogScatter");
        m_computeDepthTiles = assetDatabase->Find<Shader>("CS_VolumeFogDepthMax");
        m_shaderComposite = assetDatabase->Find<Shader>("SH_VS_VolumeFogComposite");

        auto hash = HashCache::Get();

        m_volumeResources = CreateRef<ConstantBuffer>(BufferLayout(
        {
            { ElementType::Float4, hash->pk_Volume_WindDir },
            { ElementType::Float,  hash->pk_Volume_ConstantFog },
            { ElementType::Float,  hash->pk_Volume_HeightFogExponent },
            { ElementType::Float,  hash->pk_Volume_HeightFogOffset },
            { ElementType::Float,  hash->pk_Volume_HeightFogAmount },
            { ElementType::Float,  hash->pk_Volume_Density },
            { ElementType::Float,  hash->pk_Volume_Intensity },
            { ElementType::Float,  hash->pk_Volume_Anisotropy },
            { ElementType::Float,  hash->pk_Volume_NoiseFogAmount },
            { ElementType::Float,  hash->pk_Volume_NoiseFogScale },
            { ElementType::Float,  hash->pk_Volume_WindSpeed },
        }));

        OnUpdateParameters(config);

        auto cmd = GraphicsAPI::GetCommandBuffer();
        cmd->SetImage(hash->pk_Volume_Inject, m_volumeInject.get());
        cmd->SetImage(hash->pk_Volume_Scatter, m_volumeScatter.get());
        cmd->SetTexture(hash->pk_Volume_InjectRead, m_volumeInject.get());
        cmd->SetTexture(hash->pk_Volume_ScatterRead, m_volumeScatter.get());
        cmd->SetBuffer(hash->pk_VolumeResources, m_volumeResources->GetBuffer());
        cmd->SetBuffer(hash->pk_VolumeMaxDepths, m_depthTiles.get());
    }

    void PassVolumeFog::Render(CommandBuffer* cmd, RenderTexture* destination, const uint3& resolution)
    {
        auto depthCountX = (uint)std::ceilf(resolution.x / 32.0f);
        auto depthCountY = (uint)std::ceilf(resolution.y / 32.0f);
        auto groupsInject = uint3(VolumeResolution.x / InjectThreadCount.x, VolumeResolution.y / InjectThreadCount.y, VolumeResolution.z / InjectThreadCount.z);
        auto groupsScatter = uint3(VolumeResolution.x / ScatterThreadCount.x, VolumeResolution.y / ScatterThreadCount.y, 1);

        cmd->Clear(m_depthTiles.get(), 0, sizeof(uint32_t) * VolumeResolution.x * VolumeResolution.y, 0u);

        cmd->Dispatch(m_computeDepthTiles, 0, { depthCountX, depthCountY, 1 });
        cmd->Barrier(m_depthTiles.get(), MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::ComputeRead);

        cmd->Dispatch(m_computeInject, 0, groupsInject);
        cmd->Barrier(m_volumeScatter.get(), MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::ComputeRead);

        cmd->Dispatch(m_computeScatter, 0, groupsScatter);
        cmd->Barrier(m_volumeInject.get(), MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::FragmentTexture);

        cmd->SetRenderTarget(destination, { 0 }, false, true);
        cmd->Blit(m_shaderComposite, 0);
    }

    void PassVolumeFog::OnUpdateParameters(const ApplicationConfig* config)
    {
        auto hash = HashCache::Get();
        m_volumeResources->Set<float4>(hash->pk_Volume_WindDir, float4(config->VolumeWindDirection.value, 0.0f));
        m_volumeResources->Set<float>(hash->pk_Volume_ConstantFog, config->VolumeConstantFog);
        m_volumeResources->Set<float>(hash->pk_Volume_HeightFogExponent, config->VolumeHeightFogExponent);
        m_volumeResources->Set<float>(hash->pk_Volume_HeightFogOffset, config->VolumeHeightFogOffset);
        m_volumeResources->Set<float>(hash->pk_Volume_HeightFogAmount, config->VolumeHeightFogAmount);
        m_volumeResources->Set<float>(hash->pk_Volume_Density, config->VolumeDensity);
        m_volumeResources->Set<float>(hash->pk_Volume_Intensity, config->VolumeIntensity);
        m_volumeResources->Set<float>(hash->pk_Volume_Anisotropy, config->VolumeAnisotropy);
        m_volumeResources->Set<float>(hash->pk_Volume_NoiseFogAmount, config->VolumeNoiseFogAmount);
        m_volumeResources->Set<float>(hash->pk_Volume_NoiseFogScale, config->VolumeNoiseFogScale);
        m_volumeResources->Set<float>(hash->pk_Volume_WindSpeed, config->VolumeWindSpeed);
        m_volumeResources->FlushBuffer();
    }
}