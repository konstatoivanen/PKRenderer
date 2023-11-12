#include "PrecompiledHeader.h"
#include <KTX/ktx.h>
#include "Rendering/RHI/Vulkan/Objects/VulkanTexture.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanEnumConversion.h"
#include "Rendering/RHI/Driver.h"
#include "Texture.h"

using namespace PK::Core;
using namespace PK::Core::Services;
using namespace PK::Utilities;
using namespace PK::Rendering;
using namespace PK::Rendering::RHI;
using namespace PK::Rendering::RHI::Objects;
using namespace PK::Rendering::RHI::Vulkan::Objects;

namespace PK::Rendering::RHI::Objects
{
    Ref<Texture> Texture::Create(const TextureDescriptor& descriptor, const char* name)
    {
        auto api = Driver::Get()->GetAPI();

        switch (api)
        {
            case APIType::Vulkan: return CreateRef<VulkanTexture>(descriptor, name);
        }

        return nullptr;
    }

    void Texture::Import(const char* filepath)
    {
        m_name = std::filesystem::path(GetFileName()).stem().string();

        ktxTexture2* ktxTex2;

        TextureDescriptor descriptor{};

        auto result = ktxTexture2_CreateFromNamedFile(filepath, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex2);

        if (result != KTX_SUCCESS)
        {
            PK_THROW_ERROR(ktxErrorString(result));
        }

        descriptor.usage = TextureUsage::DefaultDisk;
        descriptor.resolution = { ktxTex2->baseWidth, ktxTex2->baseHeight, ktxTex2->baseDepth };
        descriptor.levels = ktxTex2->numLevels;
        descriptor.layers = ktxTex2->numLayers;
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
        Validate(descriptor);

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

        Driver::Get()->GetQueues()->GetCommandBuffer(QueueType::Transfer)->UploadTexture(this, ktxTextureData, ktxTextureSize, ranges.data(), (uint32_t)ranges.size());

        ktxTexture_Destroy(ktxTexture(ktxTex2));
    }
}

template<>
bool AssetImporters::IsValidExtension<Texture>(const std::filesystem::path& extension) { return extension.compare(".ktx2") == 0; }

template<>
Ref<Texture> AssetImporters::Create()
{
    auto api = Driver::Get()->GetAPI();

    switch (api)
    {
        case APIType::Vulkan: return CreateRef<VulkanTexture>();
    }

    return nullptr;
}
