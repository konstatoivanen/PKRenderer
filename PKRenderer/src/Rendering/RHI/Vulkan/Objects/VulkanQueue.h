#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/RHI/Objects/QueueSet.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanStructs.h"
#include "Rendering/RHI/Vulkan/Services/VulkanCommandBufferPool.h"

namespace PK::Rendering::RHI::Vulkan::Objects
{
    class VulkanQueue : public PK::Utilities::NoCopy
    {
        constexpr static const uint32_t MAX_SEMAPHORES = 16u;
        constexpr static const uint32_t MAX_DEPENDENCIES = (uint32_t)QueueType::MaxCount;

        public:
            VulkanQueue(const VkDevice device, VkQueueFlags flags, uint32_t queueFamily, const Objects::VulkanServiceContext& services, uint32_t queueIndex = 0u);
            ~VulkanQueue();

            VkResult Present(VkSwapchainKHR swapchain, uint32_t imageIndex, VkSemaphore waitSignal = VK_NULL_HANDLE);
            VkResult Submit(Objects::VulkanCommandBuffer* commandBuffer, VkSemaphore* outSignal = nullptr);
            VkResult BindSparse(VkBuffer buffer, const VkSparseMemoryBind* binds, uint32_t bindCount);
            VkSemaphore QueueSignal(VkPipelineStageFlags flags);
            void QueueWait(VkSemaphore semaphore, VkPipelineStageFlags flags);
            void QueueWait(VulkanQueue* other, int32_t timelineOffset = 0);

            inline VkSemaphore GetNextSemaphore() { return m_semaphores[m_semaphoreIndex++ % MAX_SEMAPHORES]; }
            constexpr VkQueue GetNative() const { return m_queue; }
            constexpr uint32_t GetFamily() const { return m_family; }
            constexpr VkPipelineStageFlags GetCapabilityFlags() const { return m_capabilityFlags; }
            FenceRef GetFenceRef(int32_t timelineOffset = 0) const;

            PK::Utilities::Scope<Services::VulkanCommandBufferPool> commandPool = nullptr;
            PK::Utilities::Scope<Services::VulkanBarrierHandler> barrierHandler = nullptr;

        private:
            const VkDevice m_device;
            const uint32_t m_family = 0u;
            const uint32_t m_queueIndex = 0u;
            VkPipelineStageFlags m_capabilityFlags = 0u;
            VkQueue m_queue = VK_NULL_HANDLE;

            VulkanTimelineSemaphore m_timeline{};
            VulkanTimelineSemaphore m_waitTimelines[MAX_DEPENDENCIES]{};

            VkSemaphore m_semaphores[MAX_SEMAPHORES] = {};
            uint32_t m_semaphoreIndex = 0u;
    };

    class VulkanQueueSet : public RHI::Objects::QueueSet
    {
        public:
            struct Initializer
            {
                float priorities[(uint32_t)QueueType::MaxCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
                uint32_t queueFamilies[(uint32_t)QueueType::MaxCount]{};
                uint32_t typeIndices[(uint32_t)QueueType::MaxCount]{};
                uint32_t queueCount = 0u;
                std::vector<VkDeviceQueueCreateInfo> createInfos;
                std::vector<VkQueueFamilyProperties> familyProperties;
                Initializer(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
            };

            VulkanQueueSet(VkDevice device, const Initializer& initializer, const Objects::VulkanServiceContext& services);

            inline VulkanQueue* GetQueue(QueueType type) { return m_queues[m_queueIndices[(uint32_t)type]].get(); }
            constexpr const VulkanQueueFamilies& GetSelectedFamilies() const { return m_selectedFamilies; }
            inline CommandBuffer* GetCommandBuffer(QueueType type) final { return GetQueue(type)->commandPool->GetCurrent(); }
            
            VkResult SubmitCurrent(QueueType type, VkSemaphore* outSignal = nullptr);
            CommandBuffer* Submit(QueueType type) final;
            void Sync(QueueType from, QueueType to, int32_t submitOffset = 0) final;
            void Wait(QueueType from, QueueType to, int32_t submitOffset = 0) final;
            void Transfer(QueueType from, QueueType to) final;
            inline FenceRef GetFenceRef(QueueType type, int32_t submitOffset = 0) final { return GetQueue(type)->GetFenceRef(submitOffset); }
            void Prune();

        private:
            PK::Utilities::Scope<VulkanQueue> m_queues[MAX_DEPENDENCIES]{};
            uint32_t m_queueIndices[MAX_DEPENDENCIES]{};
            VulkanQueueFamilies m_selectedFamilies{};
    };
}