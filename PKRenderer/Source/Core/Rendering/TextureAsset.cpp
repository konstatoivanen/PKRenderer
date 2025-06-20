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
        TextureUploadRange* ranges = PK_STACK_ALLOC(TextureUploadRange, descriptor.levels);

        auto faces = ktxTex2->isCubemap ? ktxTex2->numFaces : 1u;

        // KTX 2 stores all levels in tightly packed form. no need to iterate on other data.
        for (auto level = 0u; level < descriptor.levels; ++level)
        {
            ktx_size_t offset;
            PK_THROW_ASSERT(ktxTexture_GetImageOffset(ktxTexture(ktxTex2), level, 0, 0, &offset) == KTX_SUCCESS, "Failed to get image buffer offset");

            auto& range = ranges[level];
            range.bufferOffset = (uint32_t)offset;
            range.level = level;
            range.layer = 0;
            range.layers = ktxTex2->isCubemap ? descriptor.layers * faces : descriptor.layers;
            range.offset = PK_UINT3_ZERO;
            range.extent =
            {
                descriptor.resolution.x > 1 ? descriptor.resolution.x >> level : 1,
                descriptor.resolution.y > 1 ? descriptor.resolution.y >> level : 1,
                descriptor.resolution.z > 1 ? descriptor.resolution.z >> level : 1
            };
        }

        RHI::GetCommandBuffer(QueueType::Transfer)->UploadTexture(m_texture.get(), ktxTextureData, ktxTextureSize, ranges, descriptor.levels);

        ktxTexture_Destroy(ktxTexture(ktxTex2));
    }
}

template<>
bool PK::Asset::IsValidExtension<PK::TextureAsset>(const char* extension) { return strcmp(extension, ".ktx2") == 0; }

template<>
PK::TextureAssetRef PK::Asset::Create<PK::TextureAsset>() { return CreateRef<PK::TextureAsset>(); }
