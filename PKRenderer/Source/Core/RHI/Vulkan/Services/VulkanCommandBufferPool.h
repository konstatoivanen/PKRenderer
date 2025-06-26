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
            VulkanCommandBufferPool(VkDevice device, const VulkanServiceContext& services, uint32_t queueFamily);
            ~VulkanCommandBufferPool();
    
            VulkanCommandBuffer* GetCurrent();
            VulkanCommandBuffer* EndCurrent();
            void Prune(bool all);

        private:
            constexpr static const uint32_t MAX_COMMANDBUFFERS = 24u;
            uint32_t m_queueFamily;
            VkDevice m_device;
            VkCommandPool m_pool;
            VkFence m_fences[MAX_COMMANDBUFFERS]{};
            VkCommandBuffer m_commandBuffers[MAX_COMMANDBUFFERS]{};
            VulkanCommandBuffer m_wrappers[MAX_COMMANDBUFFERS]{};
            VulkanRenderState m_primaryRenderState;
            VulkanCommandBuffer* m_current = nullptr;
    };
}
