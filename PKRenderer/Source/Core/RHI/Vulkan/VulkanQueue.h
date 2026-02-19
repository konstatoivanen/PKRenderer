#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanRenderState.h"
#include "Core/RHI/Vulkan/VulkanCommandBuffer.h"

namespace PK
{
    // Queue selection is required for device context.
    // This deferred initializer pre-selects queues for device.
    struct VulkanQueueSetInitializer
    {
        float priorities[(uint32_t)QueueType::MaxCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
        uint32_t queueFamilies[(uint32_t)QueueType::MaxCount]{};
        uint32_t typeIndices[(uint32_t)QueueType::MaxCount]{};
        const char* names[(uint32_t)QueueType::MaxCount]{};
        VkDeviceQueueCreateInfo createInfos[(uint32_t)QueueType::MaxCount]{};
        VkQueueFamilyProperties familyProperties[(uint32_t)QueueType::MaxCount]{};
        uint32_t queueCount = 0u;

        VulkanQueueSetInitializer(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    };

    class VulkanQueue : public NoCopy
    {
        constexpr static const uint32_t MAX_DEPENDENCIES = ((uint32_t)QueueType::MaxCount + 1u);

        public:
            VulkanQueue(const VkDevice device, VkQueueFlags flags, uint32_t queueFamily, VulkanServiceContext& services, uint32_t queueIndex, const char* name);
            ~VulkanQueue();

            inline VkSemaphore GetNextSemaphore() { return m_semaphores[m_semaphoreIndex++ % PK_VK_QUEUE_SEMAPHORE_COUNT]; }
            constexpr VkQueue GetNative() const { return m_queue; }
            constexpr uint32_t GetFamily() const { return m_family; }
            constexpr VkPipelineStageFlags GetCapabilityFlags() const { return m_capabilityFlags; }
            inline VulkanBarrierHandler* GetBarrierHandler() { return &m_barrierHandler; }
            FenceRef GetFenceRef(int32_t timelineOffset = 0) const;

            VulkanCommandBuffer* GetCommandBuffer();
            VkResult Submit(VkSemaphore* outSignal = nullptr);
            VkResult Present(VkSwapchainKHR swapchain, uint32_t imageIndex, uint64_t presentId, VkSemaphore waitSignal = VK_NULL_HANDLE);
            VkResult BindSparse(VkBuffer buffer, const VkSparseMemoryBind* binds, uint32_t bindCount);
            VkSemaphore QueueSignal(VkPipelineStageFlags flags);
            void QueueWait(VkSemaphore semaphore, VkPipelineStageFlags flags);
            void QueueWait(VulkanQueue* other, int32_t timelineOffset = 0);
            void WaitCommandBuffers(bool waitAll);
            void Prune();

        private:
            const VkDevice m_device;
            const uint32_t m_family;
            const uint32_t m_queueIndex;
            const VkPipelineStageFlags m_capabilityFlags;

            VulkanBarrierHandler m_barrierHandler;
            VulkanRenderState m_renderState;

            VkQueue m_queue = VK_NULL_HANDLE;
            VkCommandPool m_commandPool = VK_NULL_HANDLE;

            VulkanTimelineSemaphore m_timeline{};
            VulkanTimelineSemaphore m_waitTimelines[MAX_DEPENDENCIES]{};
            VkSemaphore m_semaphores[PK_VK_QUEUE_SEMAPHORE_COUNT] = {};
            VkFence m_commandFences[PK_VK_MAX_COMMAND_BUFFERS]{};
            VkCommandBuffer m_commandBuffers[PK_VK_MAX_COMMAND_BUFFERS]{};
            VulkanCommandBuffer m_commandWrappers[PK_VK_MAX_COMMAND_BUFFERS]{};

            VulkanCommandBuffer* m_currentCommandBuffer = nullptr;
            uint32_t m_semaphoreIndex = 0u;
    };

    class VulkanQueueSet : public RHIQueueSet
    {
        public:
            VulkanQueueSet(VkDevice device, const VulkanQueueSetInitializer& initializer, const VulkanServiceContext& services);

            inline VulkanQueue* GetQueue(QueueType type) { return m_queues[m_queueIndices[(uint32_t)type]].get(); }
            constexpr const VulkanQueueFamilies& GetSelectedFamilies() const { return m_selectedFamilies; }
            inline RHICommandBuffer* GetCommandBuffer(QueueType type) final { return GetQueue(type)->GetCommandBuffer(); }
            
            VkResult SubmitCurrent(QueueType type, VkSemaphore* outSignal = nullptr);
            RHICommandBuffer* Submit(QueueType type) final;
            void Wait(QueueType to, QueueType from, int32_t submitOffset = 0) final;
            inline FenceRef GetFenceRef(QueueType type, int32_t submitOffset = 0) final { return GetQueue(type)->GetFenceRef(submitOffset); }
            inline FenceRef GetLastSubmitFenceRef() final { return m_lastSubmitFence; }
            void Prune();

        private:
            Unique<VulkanQueue> m_queues[(uint32_t)QueueType::MaxCount]{};
            uint32_t m_queueIndices[(uint32_t)QueueType::MaxCount]{};
            VulkanQueueFamilies m_selectedFamilies{};
            FenceRef m_lastSubmitFence;
    };
}
