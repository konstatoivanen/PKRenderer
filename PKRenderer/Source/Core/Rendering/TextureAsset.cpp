#include "PrecompiledHeader.h"
#include <PKAssets/PKAsset.h>
#include <PKAssets/PKAssetLoader.h>
#include "Core/Base/Memory.h"
#include "Core/ControlFlow/FenceRef.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "TextureAsset.h"

namespace PK
{
    TextureAsset::TextureAsset(const char* filepath)
    {
        PKAssets::PKAssetStream asset{};
        PK_FATAL_ASSERT(PKAssets::OpenAssetStream(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_FATAL_ASSERT(asset.header.type == PKAssets::PKAssetType::Texture, "Trying to read a texture from a non texture file!")

        PKAssets::PKTexture texture;
        PKAssets::StreamAsTexture(&asset, &texture);

        auto graph = RHI::GetGraph();

        auto stage = graph->AcquireStagingBuffer(texture.dataSize);
        PKAssets::StreamData(&asset, stage->GetMappedPtr(), texture.data.offset, texture.dataSize);

        TextureDescriptor descriptor{};
        descriptor.usage = TextureUsage::Default;
        descriptor.resolution[0] = texture.resolution[0];
        descriptor.resolution[1] = texture.resolution[1];
        descriptor.resolution[2] = texture.resolution[2];
        descriptor.levels = (uint8_t)texture.levels;
        descriptor.layers = texture.layers;
        descriptor.format = texture.format;
        descriptor.type = texture.type;

        m_texture = RHI::CreateTexture(descriptor, String::ToFilePathStem<64>(filepath));

        auto regions = PK_STACK_ALLOC(TextureDataRegion, descriptor.levels);
        auto isCubeMap = descriptor.type == TextureType::CubemapArray || descriptor.type == TextureType::Cubemap;

        {
            uint32_t* levelOffsets = PK_STACK_ALLOC(uint32_t, descriptor.levels);
            PKAssets::StreamData(&asset, levelOffsets, texture.levelOffsets.offset, sizeof(uint32_t) * descriptor.levels);

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

        graph->CopyToTexture(QueueType::Transfer, m_texture.get(), stage, regions, descriptor.levels);
        graph->ReleaseStagingBuffer(stage);

        PKAssets::CloseAssetStream(&asset);
    }

    RHITexture* TextureAsset::GetRHI() { return m_texture.get(); }
    const RHITexture* TextureAsset::GetRHI() const { return m_texture.get(); }

    TextureAsset::operator RHITexture* () { return m_texture.get(); }
    TextureAsset::operator const RHITexture* () const { return m_texture.get(); }
}
