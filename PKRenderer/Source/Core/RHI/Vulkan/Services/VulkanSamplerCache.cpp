#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedString.h"
#include "VulkanSamplerCache.h"

namespace PK
{
    VkSampler VulkanSamplerCache::GetSampler(const SamplerDescriptor& descriptor)
    {
        return GetPooledSampler(descriptor)->sampler;
    }

    const VulkanBindHandle* VulkanSamplerCache::GetBindHandle(const SamplerDescriptor& descriptor)
    {
        auto sampler = GetPooledSampler(descriptor);
        auto index = m_samplerPool.GetIndex(sampler);
        return m_bindhandlePool[index];
    }

    VulkanSampler* VulkanSamplerCache::GetPooledSampler(const SamplerDescriptor& descriptor)
    {
        auto index = 0u;

        if (!m_samplers.AddKey(descriptor, &index))
        {
            return m_samplers[index].value;
        }

        auto sampler = &m_samplers[index].value;
        *sampler = m_samplerPool.New(m_device, descriptor, FixedString64("Sampler%u", index).c_str());

        auto bindHandle = m_bindhandlePool.New();
        bindHandle->isConcurrent = false;
        bindHandle->isTracked = false;
        bindHandle->image.image = VK_NULL_HANDLE;
        bindHandle->image.alias = VK_NULL_HANDLE;
        bindHandle->image.view = VK_NULL_HANDLE;
        bindHandle->image.sampler = (*sampler)->sampler;
        bindHandle->image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
        bindHandle->image.format = VK_FORMAT_UNDEFINED;
        bindHandle->image.extent = { 0u, 0u, 0u };
        bindHandle->image.range = { VK_IMAGE_ASPECT_NONE, 0u, VK_REMAINING_MIP_LEVELS, 0u, VK_REMAINING_ARRAY_LAYERS };
        bindHandle->image.samples = VK_SAMPLE_COUNT_1_BIT;
        return *sampler;
    }
}