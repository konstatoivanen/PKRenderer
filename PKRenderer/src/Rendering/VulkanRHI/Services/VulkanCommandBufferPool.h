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
            VulkanCommandBufferPool(const VkDevice device, const Objects::VulkanSystemContext& systems, uint32_t queueFamilyIndex);
            ~VulkanCommandBufferPool();
    
            Objects::VulkanCommandBuffer* GetCurrent();
            VkSemaphore QueueDependency(const VkSemaphore** previousDependency);
            void SubmitCurrent(VkPipelineStageFlags waitFlag, const VulkanSemaphore* waitSignal);
            void PruneStaleBuffers();
            void WaitForCompletion(bool all);
            VulkanSemaphore* AcquireRenderingFinishedSignal();
    
        private:
            constexpr static const uint32_t MAX_PRIMARY_COMMANDBUFFERS = 16u;
            constexpr static const uint32_t MAX_DEPENDENCIES = 64u;
            const VkDevice m_device;
            VkQueue m_queue;
            VkCommandPool m_pool;
            Objects::VulkanRenderState m_primaryRenderState;
            Objects::VulkanCommandBuffer m_commandBuffers[MAX_PRIMARY_COMMANDBUFFERS] = {};
            VulkanSemaphore* m_renderingFinishedSignals[MAX_PRIMARY_COMMANDBUFFERS] = {};
            VulkanSemaphore* m_dependencySignals[MAX_DEPENDENCIES] = {};

            Objects::VulkanCommandBuffer* m_current = nullptr;
            VulkanSemaphore* m_renderingFinishedSignal = {};
            VulkanSemaphore* m_currentDependencySignal = {};
            uint32_t m_dependencySignalIndex = 0u;
    };
}