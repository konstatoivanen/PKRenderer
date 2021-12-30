#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Utilities;
    using namespace Structs;

    class VulkanSamplerCache : NoCopy
    {
        public:
            VulkanSamplerCache(VkDevice device) : m_device(device) {}
            ~VulkanSamplerCache();

            VkSampler GetSampler(const SamplerDescriptor& descriptor);

        private:
            const VkDevice m_device;
            std::map<SamplerDescriptor, VulkanSampler*> m_samplers;
    };
}