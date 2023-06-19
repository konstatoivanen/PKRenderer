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
        public:
            struct SampelerKeyHash
            {
                std::size_t operator()(const Structs::SamplerDescriptor& k) const noexcept
                {
                    constexpr uint64_t seed = 18446744073709551557;
                    return PK::Utilities::HashHelpers::MurmurHash(reinterpret_cast<const void*>(&k), sizeof(Structs::SamplerDescriptor), seed);
                }
            };

            VulkanSamplerCache(VkDevice device) : m_device(device) {}
            VkSampler GetSampler(const Structs::SamplerDescriptor& descriptor);

        private:
            const VkDevice m_device;
            PK::Utilities::FixedPool<VulkanSampler, 128> m_samplerPool;
            PK::Utilities::FastMap<Structs::SamplerDescriptor, VulkanSampler*, SampelerKeyHash> m_samplers;
    };
}