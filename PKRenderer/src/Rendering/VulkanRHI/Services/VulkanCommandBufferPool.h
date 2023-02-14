#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Objects/VulkanCommandBuffer.h"
#include "Rendering/VulkanRHI/Objects/VulkanQueue.h"

namespace PK::Rendering::VulkanRHI::Services
{
    class VulkanCommandBufferPool : public PK::Utilities::NoCopy
    {
        public:
            VulkanCommandBufferPool(const VkDevice device, const Objects::VulkanServiceContext& systems, Objects::VulkanQueue* queue);
            ~VulkanCommandBufferPool();
    
            Objects::VulkanCommandBuffer* GetCurrent();
            void SubmitCurrent();
            void PruneStaleBuffers();
            void WaitForCompletion(bool all);

        private:
            constexpr static const uint32_t MAX_PRIMARY_COMMANDBUFFERS = 16u;
            const VkDevice m_device;
            VkCommandPool m_pool;
            Objects::VulkanQueue* m_queue;
            Objects::VulkanRenderState m_primaryRenderState;
            Objects::VulkanCommandBuffer m_commandBuffers[MAX_PRIMARY_COMMANDBUFFERS] = {};
            Objects::VulkanCommandBuffer* m_current = nullptr;
    };
}