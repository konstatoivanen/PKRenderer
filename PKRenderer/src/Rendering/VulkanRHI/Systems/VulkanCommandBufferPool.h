#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Objects/VulkanCommandBuffer.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Utilities;
    using namespace Objects;

    class VulkanCommandBufferPool 
    {
        public:
            VulkanCommandBufferPool(const VkDevice device, const VulkanSystemContext& systems, uint32_t queueFamilyIndex);
            ~VulkanCommandBufferPool();
    
            VulkanCommandBuffer* GetCurrent();
            void SubmitCurrent(VkPipelineStageFlags waitFlag, const VulkanSemaphore* waitSignal);
            void PruneStaleBuffers();
            void WaitForCompletion(bool all);
            VulkanSemaphore* AcquireRenderingFinishedSignal();
    
        private:
            static constexpr int MAX_PRIMARY_COMMANDBUFFERS = 16;
            const VkDevice m_device;
            VkQueue m_queue;
            VkCommandPool m_pool;
            VulkanRenderState m_primaryRenderState;
            VulkanCommandBuffer m_commandBuffers[MAX_PRIMARY_COMMANDBUFFERS] = {};
            Ref<VulkanSemaphore> m_renderingFinishedSignals[MAX_PRIMARY_COMMANDBUFFERS] = {};

            VulkanCommandBuffer* m_current = nullptr;
            VulkanSemaphore* m_renderingFinishedSignal = {};
    };
}