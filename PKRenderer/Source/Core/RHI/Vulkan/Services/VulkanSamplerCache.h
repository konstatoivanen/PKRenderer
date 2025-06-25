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
        static constexpr uint32_t MAX_SAMPLERS = 32u;

        public:
            VulkanSamplerCache(VkDevice device) : m_device(device) {}
            VkSampler GetSampler(const SamplerDescriptor& descriptor);
            VulkanBindHandle* GetBindHandle(const SamplerDescriptor& descriptor);
        
        private:
            VulkanSampler* GetPooledSampler(const SamplerDescriptor& descriptor);

            const VkDevice m_device;
            FixedPool<VulkanBindHandle, MAX_SAMPLERS> m_bindhandlePool;
            FixedPool<VulkanSampler, MAX_SAMPLERS> m_samplerPool;
            FixedMap<SamplerDescriptor, VulkanSampler*, MAX_SAMPLERS, SampelerKeyHash> m_samplers;
    };
}