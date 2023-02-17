#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Services/VulkanCommandBufferPool.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    class VulkanQueue : public PK::Utilities::NoCopy
    {
        constexpr static const uint32_t MAX_SEMAPHORES = 16u;
        constexpr static const uint32_t MAX_DEPENDENCIES = (uint32_t)Structs::QueueType::MaxCount;

        public:
            VulkanQueue(const VkDevice device, VkQueueFlags flags, uint32_t queueFamily, uint32_t queueIndex);
            ~VulkanQueue();

            VkResult Present(VkSwapchainKHR swapchain, uint32_t imageIndex, VkSemaphore waitSignal = VK_NULL_HANDLE);
            VkResult Submit(Objects::VulkanCommandBuffer* commandBuffer, VkPipelineStageFlags flags, bool waitForPrevious, VkSemaphore* outSignal = nullptr);
            VkResult BindSparse(VkBuffer buffer, const VkSparseMemoryBind* binds, uint32_t bindCount);
            VkResult QueueWait(VkSemaphore semaphore, VkPipelineStageFlags flags);
            VkSemaphore QueueSignal(VkPipelineStageFlags flags);
            void QueueWait(VulkanQueue* other);

            inline VkSemaphore GetNextSemaphore() { return m_semaphores[m_semaphoreIndex++ % MAX_SEMAPHORES]; }
            constexpr VkQueue GetNative() const { return m_queue; }
            constexpr uint32_t GetFamily() const { return m_family; }
            constexpr VkPipelineStageFlags GetCapabilityFlags() const { return m_capabilityFlags; }

            constexpr VkFence GetLastSubmitFence() const { return m_lastSubmitFence; }
            constexpr ExecutionGate GetLastSubmitGate() const { return m_lastSubmitGate; }

        private:
            struct TimelineSemaphore
            {
                VkSemaphore semaphore = VK_NULL_HANDLE;
                VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_NONE;
                uint64_t counter = 0ull;
            };

            const VkDevice m_device;
            VkPipelineStageFlags m_capabilityFlags = 0u;
            uint32_t m_family = 0u;
            uint32_t m_queueIndex = 0u;
            VkQueue m_queue = VK_NULL_HANDLE;

            VkFence m_lastSubmitFence = VK_NULL_HANDLE;
            ExecutionGate m_lastSubmitGate{};
            
            TimelineSemaphore m_timeline{};
            TimelineSemaphore m_waitTimelines[MAX_DEPENDENCIES]{};

            VkSemaphore m_semaphores[MAX_SEMAPHORES] = {};
            uint32_t m_semaphoreIndex = 0u;
    };

    class VulkanQueueSet : public PK::Utilities::NoCopy
    {
        constexpr static const uint32_t MAX_DEPENDENCIES = (uint32_t)Structs::QueueType::MaxCount;

        public:
            struct Initializer
            {
                float priorities[(uint32_t)Structs::QueueType::MaxCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
                uint32_t queueIndices[(uint32_t)Structs::QueueType::MaxCount]{};
                uint32_t queueFamilies[(uint32_t)Structs::QueueType::MaxCount]{};
                uint32_t typeIndices[(uint32_t)Structs::QueueType::MaxCount]{};
                uint32_t queueCount = 0u;
                std::vector<VkDeviceQueueCreateInfo> createInfos;
                std::vector<VkQueueFamilyProperties> familyProperties;
                Initializer(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
            };

            VulkanQueueSet(VkDevice device, const Initializer& initializer, const Objects::VulkanServiceContext& services);
            ~VulkanQueueSet();
            inline VulkanQueue* GetQueue(Structs::QueueType type) const { return m_queues[m_queueIndices[(uint32_t)type]]; }
            inline Services::VulkanCommandBufferPool* GetCommandPool(Structs::QueueType type) const { return m_commandBufferPools[m_queueIndices[(uint32_t)type]]; }
            inline VulkanCommandBuffer* GetCommandBuffer(Structs::QueueType type) const { return GetCommandPool(type)->GetCurrent(); }
            inline VulkanCommandBuffer* EndCommandBuffer(Structs::QueueType type) const { return GetCommandPool(type)->EndCurrent(); }
            inline VkResult SubmitCurrent(Structs::QueueType type, VkPipelineStageFlags flags, bool waitForPrevious, VkSemaphore* outSignal = nullptr)
            {
                return GetQueue(type)->Submit(EndCommandBuffer(type), flags, waitForPrevious, outSignal);
            }

        private:
            Services::VulkanCommandBufferPool* m_commandBufferPools[MAX_DEPENDENCIES]{};
            VulkanQueue* m_queues[MAX_DEPENDENCIES]{};
            uint32_t m_queueIndices[MAX_DEPENDENCIES]{};
    };
}