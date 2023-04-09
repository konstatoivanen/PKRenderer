#include "PrecompiledHeader.h"
#include "VulkanSamplerCache.h"

namespace PK::Rendering::VulkanRHI::Services
{
    using namespace Structs;

    VulkanSamplerCache::~VulkanSamplerCache()
    {
        for (auto& kv : m_samplers)
        {
            delete kv.second;
        }
    }

    VkSampler VulkanSamplerCache::GetSampler(const SamplerDescriptor& descriptor)
    {
        auto iterator = m_samplers.find(descriptor);

        if (iterator != m_samplers.end())
        {
            return iterator->second->sampler;
        }

        auto sampler = new VulkanSampler(m_device, descriptor);
        m_samplers[descriptor] = sampler;
        return sampler->sampler;
    }
}