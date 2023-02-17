#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Objects/VulkanCommandBuffer.h"

namespace PK::Rendering::VulkanRHI::Services
{
    class VulkanCommandBufferPool : public PK::Utilities::NoCopy
    {
        public:
            VulkanCommandBufferPool(VkDevice device, const Objects::VulkanServiceContext& services, uint32_t queueFamily);
            ~VulkanCommandBufferPool();
    
            Objects::VulkanCommandBuffer* GetCurrent();
            Objects::VulkanCommandBuffer* EndCurrent();
            void PruneStaleBuffers();
            void WaitForCompletion(bool all);

        private:
            constexpr static const uint32_t MAX_PRIMARY_COMMANDBUFFERS = 16u;
            VkDevice m_device;
            VkCommandPool m_pool;
            Objects::VulkanRenderState m_primaryRenderState;
            Objects::VulkanCommandBuffer m_commandBuffers[MAX_PRIMARY_COMMANDBUFFERS]{};
            Objects::VulkanCommandBuffer* m_current = nullptr;
    };
}