#include "PrecompiledHeader.h"
#include <KTX/ktx.h>
#include <filesystem>
#include "Core/CLI/Log.h"
#include "Graphics/RHI/RHITexture.h"
#include "Graphics/RHI/RHICommandBuffer.h"
#include "Graphics/RHI/Vulkan/VulkanCommon.h"
#include "TextureAsset.h"

namespace PK::Graphics
{
    using namespace PK::Utilities;
    using namespace PK::Graphics::RHI;

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
        //@TODO Not a cool dependency
        descriptor.format = RHI::Vulkan::EnumConvert::GetTextureFormat((VkFormat)ktxTex2->vkFormat);

        if (ktxTex2->isCubemap && ktxTex2->isArray)
        {
            descriptor.samplerType = SamplerType::CubemapArray;
        }
        else if (ktxTex2->isCubemap)
        {
            descriptor.samplerType = SamplerType::Cubemap;
        }
        else if (ktxTex2->isArray)
        {
            descriptor.samplerType = SamplerType::Sampler2DArray;
        }
        else if (ktxTex2->baseDepth > 1)
        {
            descriptor.samplerType = SamplerType::Sampler3D;
        }

        descriptor.sampler.anisotropy = 16.0f;
        descriptor.sampler.filterMin = ktxTex2->numLevels > 1 ? FilterMode::Trilinear : FilterMode::Bilinear;
        descriptor.sampler.filterMag = ktxTex2->numLevels > 1 ? FilterMode::Trilinear : FilterMode::Bilinear;
        descriptor.sampler.wrap[0] = WrapMode::Repeat;
        descriptor.sampler.wrap[1] = WrapMode::Repeat;
        descriptor.sampler.wrap[2] = WrapMode::Repeat;

        m_texture = RHICreateTexture(descriptor, std::filesystem::path(GetFileName()).stem().string().c_str());

        ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture(ktxTex2));
        ktx_size_t ktxTextureSize = ktxTex2->dataSize;
        std::vector<ImageUploadRange> ranges;

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
            range.offset = PK::Math::PK_UINT3_ZERO;
            range.extent =
            {
                descriptor.resolution.x > 1 ? descriptor.resolution.x >> level : 1,
                descriptor.resolution.y > 1 ? descriptor.resolution.y >> level : 1,
                descriptor.resolution.z > 1 ? descriptor.resolution.z >> level : 1
            };
        }

        RHIGetCommandBuffer(QueueType::Transfer)->UploadTexture(m_texture.get(), ktxTextureData, ktxTextureSize, ranges.data(), (uint32_t)ranges.size());

        ktxTexture_Destroy(ktxTexture(ktxTex2));
    }
    
    Texture* TextureAsset::GetRHI() { return m_texture.get(); }
    const Texture* TextureAsset::GetRHI() const { return m_texture.get(); }
    TextureAsset::operator Texture* () { return m_texture.get(); }
    TextureAsset::operator const Texture* () const { return m_texture.get(); }
}


template<>
bool PK::Core::Assets::Asset::IsValidExtension<PK::Graphics::TextureAsset>(const std::string& extension) { return extension.compare(".ktx2") == 0; }

template<>
PK::Graphics::TextureAssetRef PK::Core::Assets::Asset::Create<PK::Graphics::TextureAsset>() { return PK::Utilities::CreateRef<PK::Graphics::TextureAsset>(); }
