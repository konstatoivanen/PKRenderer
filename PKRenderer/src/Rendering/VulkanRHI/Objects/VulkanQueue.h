#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Objects/VulkanCommandBuffer.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    class VulkanQueue : public PK::Utilities::NoCopy
    {
        public:
            struct SignalInfo
            {
                VkSemaphore signal = VK_NULL_HANDLE;
                const VkSemaphore* waits = nullptr;
                const VkPipelineStageFlags* waitFlags = nullptr;
                uint32_t waitCount = 0u;
            };

            constexpr static const uint32_t MAX_DEPENDENCIES = 64u;
            constexpr static const uint32_t MAX_QUEUED_DEPENDENCIES = 8u;

            VulkanQueue(const VkDevice device, const VkQueueFamilyProperties& properties, uint32_t queueFamily, uint32_t queueIndex);
            ~VulkanQueue();

            VkResult Present(VkSwapchainKHR swapchain, uint32_t imageIndex);
            VkResult Submit(Objects::VulkanCommandBuffer* commandBuffer, VkPipelineStageFlags flags, bool waitForPrevious);
            VkResult BindSparse(VkBuffer buffer, const VkSparseMemoryBind* binds, uint32_t bindCount);
            void QueueDependency(VkPipelineStageFlags flags, SignalInfo* signalInfo, bool addNew, bool waitForPrevious);

            constexpr VkQueue GetNative() const { return m_queue; }
            constexpr uint32_t GetFamily() const { return m_family; }
            constexpr VkPipelineStageFlags GetCapabilityFlags() const { return m_capabilityFlags; }
            constexpr const VkQueueFamilyProperties& GetFamilyProperties() const { return m_familyProperties; }

        private:
            struct SignalGroup
            {
                VkSemaphore semaphores[MAX_QUEUED_DEPENDENCIES] = {};
                VkPipelineStageFlags flags[MAX_QUEUED_DEPENDENCIES] = {};
                uint32_t count = 0u;
            };

            const VkDevice m_device;
            VkQueueFamilyProperties m_familyProperties;
            VkPipelineStageFlags m_capabilityFlags = 0u;
            uint32_t m_family = 0u;
            uint32_t m_queueIndex = 0u;
            VkQueue m_queue = VK_NULL_HANDLE;

            VulkanSemaphore* m_semaphores[MAX_DEPENDENCIES] = {};
            uint32_t m_semaphoreIndex = 0u;

            SignalGroup m_signalGroups[2] = {};
    };

    class VulkanQueueSet : public PK::Utilities::NoCopy
    {
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

            VulkanQueueSet(VkDevice device, const Initializer& initializer);
            ~VulkanQueueSet();
            VulkanQueue* GetQueue(Structs::QueueType type) const { return m_queues[m_queueIndices[(uint32_t)type]]; }
        
        private:
            VulkanQueue* m_queues[(uint32_t)Structs::QueueType::MaxCount] = {};
            uint32_t m_queueIndices[(uint32_t)Structs::QueueType::MaxCount] = {};
    };
}