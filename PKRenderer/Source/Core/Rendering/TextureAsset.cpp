#include "PrecompiledHeader.h"
#include <KTX/ktx.h>
#include <filesystem>
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "TextureAsset.h"

namespace PK
{
    RHITexture* TextureAsset::GetRHI() { return m_texture.get(); }
    const RHITexture* TextureAsset::GetRHI() const { return m_texture.get(); }

    TextureAsset::operator RHITexture* () { return m_texture.get(); }
    TextureAsset::operator const RHITexture* () const { return m_texture.get(); }

    void TextureAsset::AssetImport(const char* filepath)
    {
        ktxTexture2* ktxTex2;

        auto result = ktxTexture2_CreateFromNamedFile(filepath, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex2);

        if (result != KTX_SUCCESS)
        {
            PK_THROW_ERROR(ktxErrorString(result));
        }

        TextureDescriptor descriptor{};
        descriptor.usage = TextureUsage::DefaultDisk;
        descriptor.resolution = { ktxTex2->baseWidth, ktxTex2->baseHeight, ktxTex2->baseDepth };
        descriptor.levels = ktxTex2->numLevels;
        descriptor.layers = ktxTex2->numLayers;
        descriptor.format = VulkanEnumConvert::GetTextureFormat((VkFormat)ktxTex2->vkFormat);

        if (ktxTex2->isCubemap && ktxTex2->isArray)
        {
            descriptor.type = TextureType::CubemapArray;
        }
        else if (ktxTex2->isCubemap)
        {
            descriptor.type = TextureType::Cubemap;
        }
        else if (ktxTex2->isArray)
        {
            descriptor.type = TextureType::Texture2DArray;
        }
        else if (ktxTex2->baseDepth > 1)
        {
            descriptor.type = TextureType::Texture3D;
        }

        descriptor.sampler.anisotropy = 16.0f;
        descriptor.sampler.filterMin = ktxTex2->numLevels > 1 ? FilterMode::Trilinear : FilterMode::Bilinear;
        descriptor.sampler.filterMag = ktxTex2->numLevels > 1 ? FilterMode::Trilinear : FilterMode::Bilinear;
        descriptor.sampler.wrap[0] = WrapMode::Repeat;
        descriptor.sampler.wrap[1] = WrapMode::Repeat;
        descriptor.sampler.wrap[2] = WrapMode::Repeat;

        m_texture = RHI::CreateTexture(descriptor, std::filesystem::path(GetFileName()).stem().string().c_str());

        ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture(ktxTex2));
        ktx_size_t ktxTextureSize = ktxTex2->dataSize;
        std::vector<TextureUploadRange> ranges;

        auto faces = ktxTex2->isCubemap ? ktxTex2->numFaces : 1u;

        for (auto layer = 0u; layer < descriptor.layers; ++layer)
        for (auto level = 0u; level < descriptor.levels; ++level)
        for (auto face = 0u; face < faces; ++face)
        {
            ktx_size_t offset;
            PK_THROW_ASSERT(ktxTexture_GetImageOffset(ktxTexture(ktxTex2), level, layer, face, &offset) == KTX_SUCCESS, "Failed to get image buffer offset");

            ranges.push_back({});
            auto& range = ranges.back();
            range.bufferOffset = (uint32_t)offset;
            range.level = level;
            range.layer = ktxTex2->isCubemap ? ((layer * faces) + face) : layer;
            range.layers = 1;
            range.offset = PK_UINT3_ZERO;
            range.extent =
            {
                descriptor.resolution.x > 1 ? descriptor.resolution.x >> level : 1,
                descriptor.resolution.y > 1 ? descriptor.resolution.y >> level : 1,
                descriptor.resolution.z > 1 ? descriptor.resolution.z >> level : 1
            };
        }

        RHI::GetCommandBuffer(QueueType::Transfer)->UploadTexture(m_texture.get(), ktxTextureData, ktxTextureSize, ranges.data(), (uint32_t)ranges.size());

        ktxTexture_Destroy(ktxTexture(ktxTex2));
    }
}

template<>
bool PK::Asset::IsValidExtension<PK::TextureAsset>(const std::string& extension) { return extension.compare(".ktx2") == 0; }

template<>
PK::TextureAssetRef PK::Asset::Create<PK::TextureAsset>() { return CreateRef<PK::TextureAsset>(); }
