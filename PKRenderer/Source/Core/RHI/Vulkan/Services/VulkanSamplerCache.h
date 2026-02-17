#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FastMap.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    class VulkanSamplerCache : NoCopy
    {
        using SampelerKeyHash = Hash::TMurmurHash<SamplerDescriptor>;

        public:
            VulkanSamplerCache(VkDevice device) : m_device(device) {}
            VkSampler GetSampler(const SamplerDescriptor& descriptor);
            const VulkanBindHandle* GetBindHandle(const SamplerDescriptor& descriptor);
        
        private:
            VulkanSampler* GetPooledSampler(const SamplerDescriptor& descriptor);

            const VkDevice m_device;
            FixedPool<VulkanBindHandle, PK_VK_MAX_SAMPLERS> m_bindhandlePool;
            FixedPool<VulkanSampler, PK_VK_MAX_SAMPLERS> m_samplerPool;
            FixedMap<SamplerDescriptor, VulkanSampler*, PK_VK_MAX_SAMPLERS, SampelerKeyHash, 2ull> m_samplers;
    };
}
