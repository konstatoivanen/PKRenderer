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
            VulkanCommandBufferPool(VkDevice device, 
                const VulkanServiceContext& services, 
                uint32_t queueFamily, 
                VkPipelineStageFlags capabilities);
            ~VulkanCommandBufferPool();
    
            VulkanCommandBuffer* GetCurrent();
            VulkanCommandBuffer* EndCurrent();
            void Prune(bool all);
            void AllocateBuffers();

        private:
            constexpr static const uint32_t MAX_PRIMARY_COMMANDBUFFERS = 24u;
            VkDevice m_device;
            VkCommandPool m_pool;
            VkCommandBuffer m_nativeBuffers[MAX_PRIMARY_COMMANDBUFFERS]{};
            VulkanRenderState m_primaryRenderState;
            VulkanCommandBuffer m_commandBuffers[MAX_PRIMARY_COMMANDBUFFERS]{};
            VulkanCommandBuffer* m_current = nullptr;
    };
}