#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/FixedPool.h"
#include "Utilities/FastMap.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"

namespace PK::Rendering::VulkanRHI::Services
{
    class VulkanSamplerCache : PK::Utilities::NoCopy
    {
        using SampelerKeyHash = PK::Utilities::HashHelpers::TMurmurHash<Structs::SamplerDescriptor>;

        public:
            VulkanSamplerCache(VkDevice device) : m_device(device) {}
            VkSampler GetSampler(const Structs::SamplerDescriptor& descriptor);
            VulkanBindHandle* GetBindHandle(const Structs::SamplerDescriptor& descriptor);
        
        private:
            VulkanSampler* GetPooledSampler(const Structs::SamplerDescriptor& descriptor);

            const VkDevice m_device;
            PK::Utilities::FixedPool<VulkanBindHandle, 128> m_bindhandlePool;
            PK::Utilities::FixedPool<VulkanSampler, 128> m_samplerPool;
            PK::Utilities::FastMap<Structs::SamplerDescriptor, VulkanSampler*, SampelerKeyHash> m_samplers;
    };
}