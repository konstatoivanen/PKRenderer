#include "PrecompiledHeader.h"
#include "VulkanSamplerCache.h"

namespace PK::Rendering::VulkanRHI::Services
{
    using namespace Structs;

    VkSampler VulkanSamplerCache::GetSampler(const SamplerDescriptor& descriptor)
    {
        auto index = 0u;

        if (!m_samplers.AddKey(descriptor, &index))
        {
            return m_samplers.GetValueAt(index)->sampler;
        }

        auto sampler = m_samplers.GetValueAtRef(index);
        *sampler = m_samplerPool.New(m_device, descriptor);
        return (*sampler)->sampler;
    }
}