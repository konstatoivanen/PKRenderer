#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/RHI/Vulkan/VulkanRenderState.h"
#include "Core/RHI/Vulkan/VulkanCommandBuffer.h"

namespace PK
{
    class VulkanCommandBufferPool : public NoCopy
    {
        public:
            VulkanCommandBufferPool(VkDevice device, const VulkanServiceContext& services, uint32_t queueFamily, const char* name);
            ~VulkanCommandBufferPool();
    
            VulkanCommandBuffer* GetCurrent();
            VulkanCommandBuffer* EndCurrent();
            void Prune(bool all);

        private:
            uint32_t m_queueFamily;
            VkDevice m_device;
            VkCommandPool m_pool;
            VkFence m_fences[PK_VK_MAX_COMMAND_BUFFERS]{};
            VkCommandBuffer m_commandBuffers[PK_VK_MAX_COMMAND_BUFFERS]{};
            VulkanCommandBuffer m_wrappers[PK_VK_MAX_COMMAND_BUFFERS]{};
            VulkanRenderState m_primaryRenderState;
            VulkanCommandBuffer* m_current = nullptr;
    };
}
