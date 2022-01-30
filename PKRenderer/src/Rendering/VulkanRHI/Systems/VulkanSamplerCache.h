#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    class VulkanSamplerCache : PK::Utilities::NoCopy
    {
        public:
            VulkanSamplerCache(VkDevice device) : m_device(device) {}
            ~VulkanSamplerCache();

            VkSampler GetSampler(const Structs::SamplerDescriptor& descriptor);

        private:
            const VkDevice m_device;
            std::map<Structs::SamplerDescriptor, VulkanSampler*> m_samplers;
    };
}