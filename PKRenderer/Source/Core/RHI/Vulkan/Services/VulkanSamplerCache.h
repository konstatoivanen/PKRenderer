#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/Pool.h"
#include "Core/Utilities/HashMap.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanSamplerCache : NoCopy
    {
        using SampelerHash = Hash::TMurmurHash<SamplerDescriptor>;

        VulkanSamplerCache(VkDevice device) : m_device(device) {}
        VkSampler GetSampler(const SamplerDescriptor& descriptor);
        const VulkanBindHandle* GetBindHandle(const SamplerDescriptor& descriptor);
        
    private:
        VulkanSampler* GetPooledSampler(const SamplerDescriptor& descriptor);

        const VkDevice m_device;
        FixedPool<VulkanBindHandle, PK_VK_MAX_SAMPLERS> m_bindhandlePool;
        FixedPool<VulkanSampler, PK_VK_MAX_SAMPLERS> m_samplerPool;
        FixedMap<SamplerDescriptor, VulkanSampler*, PK_VK_MAX_SAMPLERS, SampelerHash, 2ull> m_samplers;
    };
}
