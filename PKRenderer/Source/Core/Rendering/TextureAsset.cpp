#include "PrecompiledHeader.h"
#include <filesystem>
#include <PKAssets/PKAsset.h>
#include <PKAssets/PKAssetLoader.h>
#include "Core/Utilities/FenceRef.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "TextureAsset.h"

namespace PK
{
    RHITexture* TextureAsset::GetRHI() { return m_texture.get(); }
    const RHITexture* TextureAsset::GetRHI() const { return m_texture.get(); }

    TextureAsset::operator RHITexture* () { return m_texture.get(); }
    TextureAsset::operator const RHITexture* () const { return m_texture.get(); }

    void TextureAsset::AssetImport(const char* filepath)
    {
        PKAssets::PKAssetStream asset;

        PK_THROW_ASSERT(PKAssets::OpenAssetStream(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_THROW_ASSERT(asset.header.type == PKAssets::PKAssetType::Texture, "Trying to read a texture from a non texture file!")

        PKAssets::PKTexture texture;
        PKAssets::StreamAsTexture(&asset, &texture);

        auto stage = RHI::AcquireStage(texture.dataSize);
        {
            auto pMapped = stage->BeginMap(0, 0);
            PKAssets::StreamData(&asset, pMapped, texture.data.offset, texture.dataSize);
            stage->EndMap(0, texture.dataSize);
        }

        TextureDescriptor descriptor{};
        descriptor.usage = TextureUsage::DefaultDisk;
        descriptor.resolution[0] = texture.resolution[0];
        descriptor.resolution[1] = texture.resolution[1];
        descriptor.resolution[2] = texture.resolution[2];
        descriptor.levels = texture.levels;
        descriptor.layers = texture.layers;
        descriptor.format = texture.format; 
        descriptor.type = texture.type;
        descriptor.sampler.anisotropy = texture.anisotropy;
        descriptor.sampler.filterMin = texture.filterMin;
        descriptor.sampler.filterMag = texture.filterMag;
        descriptor.sampler.wrap[0] = texture.wrap[0];
        descriptor.sampler.wrap[1] = texture.wrap[1];
        descriptor.sampler.wrap[2] = texture.wrap[2];

        m_texture = RHI::CreateTexture(descriptor, std::filesystem::path(GetFileName()).stem().string().c_str());

        auto regions = PK_STACK_ALLOC(TextureDataRegion, descriptor.levels);
        auto isCubeMap = descriptor.type == TextureType::CubemapArray || descriptor.type == TextureType::Cubemap;

        {
            uint32_t* levelOffsets = PK_STACK_ALLOC(uint32_t, descriptor.levels);
            PKAssets::StreamRelativePtr(&asset, levelOffsets, texture.levelOffsets, descriptor.levels);

            // Data stored packed per level. only need to define level ranges.
            for (auto level = 0u; level < descriptor.levels; ++level)
            {
                auto& region = regions[level];
                region.bufferOffset = levelOffsets[level];
                region.level = level;
                region.layer = 0;
                region.layers = isCubeMap ? descriptor.layers * 6u : descriptor.layers;
                region.offset = PK_UINT3_ZERO;
                region.extent =
                {
                    descriptor.resolution.x > 1 ? descriptor.resolution.x >> level : 1,
                    descriptor.resolution.y > 1 ? descriptor.resolution.y >> level : 1,
                    descriptor.resolution.z > 1 ? descriptor.resolution.z >> level : 1
                };
            }
        }

        RHI::GetCommandBuffer(QueueType::Transfer)->CopyToTexture(m_texture.get(), stage, regions, descriptor.levels);
        RHI::ReleaseStage(stage, RHI::GetCommandBuffer(QueueType::Transfer)->GetFenceRef());

        PKAssets::CloseAssetStream(&asset);
    }
}

template<>
bool PK::Asset::IsValidExtension<PK::TextureAsset>(const char* extension) { return strcmp(extension, ".pktexture") == 0; }

template<>
PK::TextureAssetRef PK::Asset::Create<PK::TextureAsset>() { return CreateRef<PK::TextureAsset>(); }
