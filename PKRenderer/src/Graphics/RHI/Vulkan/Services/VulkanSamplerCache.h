#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/FixedPool.h"
#include "Utilities/FastMap.h"
#include "Graphics/RHI/Vulkan/VulkanCommon.h"

namespace PK::Graphics::RHI::Vulkan::Services
{
    class VulkanSamplerCache : PK::Utilities::NoCopy
    {
        using SampelerKeyHash = PK::Utilities::Hash::TMurmurHash<SamplerDescriptor>;

        public:
            VulkanSamplerCache(VkDevice device) : m_device(device) {}
            VkSampler GetSampler(const SamplerDescriptor& descriptor);
            VulkanBindHandle* GetBindHandle(const SamplerDescriptor& descriptor);
        
        private:
            VulkanSampler* GetPooledSampler(const SamplerDescriptor& descriptor);

            const VkDevice m_device;
            PK::Utilities::FixedPool<VulkanBindHandle, 128> m_bindhandlePool;
            PK::Utilities::FixedPool<VulkanSampler, 128> m_samplerPool;
            PK::Utilities::FastMap<SamplerDescriptor, VulkanSampler*, SampelerKeyHash> m_samplers = 
                PK::Utilities::FastMap<SamplerDescriptor, VulkanSampler*, SampelerKeyHash>(16ull);
    };
}