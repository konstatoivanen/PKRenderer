#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/Objects/QueueSet.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Services/VulkanCommandBufferPool.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    class VulkanQueue : public PK::Utilities::NoCopy
    {
        constexpr static const uint32_t MAX_SEMAPHORES = 16u;
        constexpr static const uint32_t MAX_DEPENDENCIES = (uint32_t)Structs::QueueType::MaxCount;

        public:
            VulkanQueue(const VkDevice device, VkQueueFlags flags, uint32_t queueFamily, const Objects::VulkanServiceContext& services, uint32_t queueIndex = 0u);
            ~VulkanQueue();

            VkResult Present(VkSwapchainKHR swapchain, uint32_t imageIndex, VkSemaphore waitSignal = VK_NULL_HANDLE);
            VkResult Submit(Objects::VulkanCommandBuffer* commandBuffer, VkSemaphore* outSignal = nullptr);
            VkResult BindSparse(VkBuffer buffer, const VkSparseMemoryBind* binds, uint32_t bindCount);
            VkSemaphore QueueSignal(VkPipelineStageFlags flags);
            void QueueWait(VkSemaphore semaphore, VkPipelineStageFlags flags);
            void QueueWait(VulkanQueue* other);

            inline VkSemaphore GetNextSemaphore() { return m_semaphores[m_semaphoreIndex++ % MAX_SEMAPHORES]; }
            constexpr VkQueue GetNative() const { return m_queue; }
            constexpr uint32_t GetFamily() const { return m_family; }
            constexpr VkPipelineStageFlags GetCapabilityFlags() const { return m_capabilityFlags; }
            FenceRef GetFenceRef() const;

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

    class VulkanQueueSet : public PK::Rendering::Objects::QueueSet
    {
        public:
            struct Initializer
            {
                float priorities[(uint32_t)Structs::QueueType::MaxCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
                uint32_t queueFamilies[(uint32_t)Structs::QueueType::MaxCount]{};
                uint32_t typeIndices[(uint32_t)Structs::QueueType::MaxCount]{};
                uint32_t queueCount = 0u;
                std::vector<VkDeviceQueueCreateInfo> createInfos;
                std::vector<VkQueueFamilyProperties> familyProperties;
                Initializer(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
            };

            VulkanQueueSet(VkDevice device, const Initializer& initializer, const Objects::VulkanServiceContext& services);

            inline VulkanQueue* GetQueue(Structs::QueueType type) { return m_queues[m_queueIndices[(uint32_t)type]].get(); }
            constexpr const VulkanQueueFamilies& GetSelectedFamilies() const { return m_selectedFamilies; }
            inline Objects::CommandBuffer* GetCommandBuffer(Structs::QueueType type) override final { return GetQueue(type)->commandPool->GetCurrent(); }
            
            VkResult SubmitCurrent(Structs::QueueType type, VulkanBarrierInfo* barrierInfo = nullptr, VkSemaphore* outSignal = nullptr);
            Objects::CommandBuffer* Submit(Structs::QueueType type) override final;
            void Sync(Structs::QueueType from, Structs::QueueType to) override final;
            inline Structs::FenceRef GetFenceRef(Structs::QueueType type) override final { return GetQueue(type)->GetFenceRef(); }
            void Prune();

        private:
            PK::Utilities::Scope<VulkanQueue> m_queues[MAX_DEPENDENCIES]{};
            uint32_t m_queueIndices[MAX_DEPENDENCIES]{};
            VulkanQueueFamilies m_selectedFamilies{};
    };
}