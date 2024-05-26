#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FastMap.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    class VulkanSamplerCache : NoCopy
    {
        using SampelerKeyHash = Hash::TMurmurHash<SamplerDescriptor>;

        public:
            VulkanSamplerCache(VkDevice device) : m_device(device) {}
            VkSampler GetSampler(const SamplerDescriptor& descriptor);
            VulkanBindHandle* GetBindHandle(const SamplerDescriptor& descriptor);
        
        private:
            VulkanSampler* GetPooledSampler(const SamplerDescriptor& descriptor);

            const VkDevice m_device;
            FixedPool<VulkanBindHandle, 128> m_bindhandlePool;
            FixedPool<VulkanSampler, 128> m_samplerPool;
            FastMap<SamplerDescriptor, VulkanSampler*, SampelerKeyHash> m_samplers = 
                FastMap<SamplerDescriptor, VulkanSampler*, SampelerKeyHash>(16ull);
    };
}