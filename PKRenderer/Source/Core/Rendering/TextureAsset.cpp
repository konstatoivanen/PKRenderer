#include "PrecompiledHeader.h"
#include <filesystem>
#include <PKAssets/PKAsset.h>
#include <PKAssets/PKAssetLoader.h>
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
        PKAssets::PKAsset asset;

        PK_THROW_ASSERT(PKAssets::OpenAsset(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_THROW_ASSERT(asset.header->type == PKAssets::PKAssetType::Texture, "Trying to read a texture from a non texture file!")

        auto texture = PKAssets::ReadAsTexture(&asset);
        auto base = asset.rawData;
        auto levelOffsets = texture->levelOffsets.Get(base);
        auto textureData = texture->data.Get(base);
        auto textureDataSize = texture->dataSize;

        TextureDescriptor descriptor{};
        descriptor.usage = TextureUsage::DefaultDisk;
        descriptor.resolution[0] = texture->resolution[0];
        descriptor.resolution[1] = texture->resolution[1];
        descriptor.resolution[2] = texture->resolution[2];
        descriptor.levels = texture->levels;
        descriptor.layers = texture->layers;
        descriptor.format = texture->format; 
        descriptor.type = texture->type;
        descriptor.sampler.anisotropy = texture->anisotropy;
        descriptor.sampler.filterMin = texture->filterMin;
        descriptor.sampler.filterMag = texture->filterMag;
        descriptor.sampler.wrap[0] = texture->wrap[0];
        descriptor.sampler.wrap[1] = texture->wrap[1];
        descriptor.sampler.wrap[2] = texture->wrap[2];

        m_texture = RHI::CreateTexture(descriptor, std::filesystem::path(GetFileName()).stem().string().c_str());

        auto ranges = PK_STACK_ALLOC(TextureUploadRange, descriptor.levels);
        auto isCubeMap = descriptor.type == TextureType::CubemapArray || descriptor.type == TextureType::Cubemap;

        // Data stored packed per level. only need to define level ranges.
        for (auto level = 0u; level < descriptor.levels; ++level)
        {
            auto& range = ranges[level];
            range.bufferOffset = levelOffsets[level];
            range.level = level;
            range.layer = 0;
            range.layers = isCubeMap ? descriptor.layers * 6u : descriptor.layers;
            range.offset = PK_UINT3_ZERO;
            range.extent =
            {
                descriptor.resolution.x > 1 ? descriptor.resolution.x >> level : 1,
                descriptor.resolution.y > 1 ? descriptor.resolution.y >> level : 1,
                descriptor.resolution.z > 1 ? descriptor.resolution.z >> level : 1
            };
        }

        RHI::GetCommandBuffer(QueueType::Transfer)->UploadTexture(m_texture.get(), textureData, textureDataSize, ranges, descriptor.levels);

        PKAssets::CloseAsset(&asset);
    }
}

template<>
bool PK::Asset::IsValidExtension<PK::TextureAsset>(const char* extension) { return strcmp(extension, ".pktexture") == 0; }

template<>
PK::TextureAssetRef PK::Asset::Create<PK::TextureAsset>() { return CreateRef<PK::TextureAsset>(); }
