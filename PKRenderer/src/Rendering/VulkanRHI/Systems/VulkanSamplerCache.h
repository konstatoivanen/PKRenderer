#pragma once
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Core/NoCopy.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Utilities;
    using namespace Structs;

    class VulkanSamplerCache : PK::Core::NoCopy
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