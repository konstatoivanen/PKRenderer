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

        private:
            const VkDevice m_device;
            PK::Utilities::FixedPool<VulkanSampler, 128> m_samplerPool;
            PK::Utilities::FastMap<Structs::SamplerDescriptor, VulkanSampler*, SampelerKeyHash> m_samplers;
    };
}