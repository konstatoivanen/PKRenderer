#include "PrecompiledHeader.h"
#include "Core/Base/Containers/FixedString.h"
#include "VulkanSamplerCache.h"

namespace PK
{
    VulkanSamplerCache::~VulkanSamplerCache()
    {
        for (auto i = m_samplers.GetCount(); i > 0u; --i)
        {
            vkDestroySampler(m_device, m_samplers[i - 1u].value.sampler, nullptr);
        }

        m_samplers.ClearFast();
    }

    VulkanSamplerCache::Sampler* VulkanSamplerCache::GetPooledSampler(const SamplerDescriptor& descriptor)
    {
        auto index = 0u;

        if (m_samplers.AddKey(descriptor, &index))
        {
            auto sampler = &m_samplers[index].value;
            
            VkSamplerCreateInfo info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
            info.addressModeU = VulkanEnumConvert::GetSamplerAddressMode(descriptor.wrap[0]);
            info.addressModeV = VulkanEnumConvert::GetSamplerAddressMode(descriptor.wrap[1]);
            info.addressModeW = VulkanEnumConvert::GetSamplerAddressMode(descriptor.wrap[2]);
            info.minLod = descriptor.mipMin;
            info.maxLod = descriptor.mipMax <= 0.0f ? VK_LOD_CLAMP_NONE : descriptor.mipMax;
            info.mipLodBias = descriptor.mipBias;
            info.maxAnisotropy = descriptor.anisotropy;
            info.anisotropyEnable = descriptor.anisotropy > 0.0f ? VK_TRUE : VK_FALSE;
            info.unnormalizedCoordinates = !descriptor.normalized;
            info.borderColor = VulkanEnumConvert::GetBorderColor(descriptor.borderColor);
            info.mipmapMode = (uint32_t)descriptor.filterMin > (uint32_t)FilterMode::Bilinear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
            info.compareEnable = descriptor.comparison != Comparison::Off ? VK_TRUE : VK_FALSE;
            info.compareOp = VulkanEnumConvert::GetCompareOp(descriptor.comparison);
            info.magFilter = VulkanEnumConvert::GetFilterMode(descriptor.filterMag);
            info.minFilter = VulkanEnumConvert::GetFilterMode(descriptor.filterMin);

            if (info.unnormalizedCoordinates)
            {
                info.minLod = info.maxLod = 0.0f;
            }

            VK_ASSERT_RESULT_CTX(vkCreateSampler(m_device, &info, nullptr, &sampler->sampler), "Failed to create a sampler!");
            VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_SAMPLER, (uint64_t)sampler->sampler, FixedString64("Sampler%u", index).c_str());

            sampler->handle.isConcurrent = false;
            sampler->handle.isTracked = false;
            sampler->handle.image.image = VK_NULL_HANDLE;
            sampler->handle.image.alias = VK_NULL_HANDLE;
            sampler->handle.image.view = VK_NULL_HANDLE;
            sampler->handle.image.sampler = sampler->sampler;
            sampler->handle.image.format = VK_FORMAT_UNDEFINED;
            sampler->handle.image.extent = { 0u, 0u, 0u };
            sampler->handle.image.range = { VK_IMAGE_ASPECT_NONE, 0u, VK_REMAINING_MIP_LEVELS, 0u, VK_REMAINING_ARRAY_LAYERS };
            sampler->handle.image.samples = VK_SAMPLE_COUNT_1_BIT;
        }
            
        return &m_samplers[index].value;
    }
}
