#pragma once
#include "Core/Base/Types/Ref.h"
#include "Core/Base/NoCopy.h"
#include "Core/RHI/Vulkan/VulkanCommandBuffer.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/RHInterfaces.h"

namespace PK
{
    // Queue selection is required for device context.
    // This deferred initializer pre-selects queues for device.
    struct VulkanQueueSetInitializer
    {
        constexpr static const uint32_t MAX_QUEUES = (uint32_t)QueueType::EnumCount;

        float priorities[MAX_QUEUES] = { 1.0f, 1.0f, 1.0f, 1.0f };
        uint32_t queueFamilies[MAX_QUEUES]{};
        uint32_t typeIndices[MAX_QUEUES]{};
        const char* names[MAX_QUEUES]{};
        VkDeviceQueueCreateInfo createInfos[MAX_QUEUES]{};
        VkQueueFamilyProperties familyProperties[MAX_QUEUES]{};
        uint32_t queueCount = 0u;
        VulkanQueueFamilies selectedAll;
        VulkanQueueFamilies selectedRender;

        VulkanQueueSetInitializer(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    };

    struct VulkanQueue : public NoCopy
    {
        struct Timeline
        {
            VkSemaphore semaphore = VK_NULL_HANDLE;
            VkPipelineStageFlags2 stageFlags = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            uint64_t counter = 0ull;
        };

        friend struct VulkanQueueSet;
        constexpr static const uint32_t MAX_DEPENDENCIES = (uint32_t)QueueType::EnumCount + 1u;

        VulkanQueue(const VkDevice device, VkQueueFlags flags, uint32_t queueFamily, uint32_t queueIndex, const char* name);
        ~VulkanQueue();

        constexpr uint32_t GetFamily() const { return m_family; }
        constexpr bool IsRecording() const { return m_currentWrapper != nullptr && m_currentWrapper->IsRecording(); }
        FenceRef GetFenceRef(uint32_t timelineOffset) const;

        VulkanCommandBuffer* GetCommandBuffer();
        void Submit(VkSemaphore inSignal);
        void Present(VkSwapchainKHR swapchain, uint32_t imageIndex, uint64_t presentId, VkPresentModeKHR mode, VkSemaphore waitSignal);
        void AddWait(VkSemaphore semaphore, VkPipelineStageFlags2 flags, uint64_t counter);
        void AddWait(VulkanQueue* other, VkPipelineStageFlags2 flags);
        void WaitCommandBuffers(bool waitAll);

    private:
        const VkDevice m_device;
        const uint32_t m_family;
        const uint32_t m_queueIndex;
        const VkQueueFlags m_queueFlags;
        const VkAccessFlags2 m_accessFlags;
        const VkPipelineStageFlags2 m_stageFlags;
        VkBindHeapInfoEXT m_resourceHeap{};
        VkBindHeapInfoEXT m_samplerHeap{};

        VkQueue m_queue = VK_NULL_HANDLE;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;

        Timeline m_signal{};
        Timeline m_waits[MAX_DEPENDENCIES]{};
        VkCommandBuffer m_commandBuffers[PK_VK_MAX_COMMAND_BUFFERS]{};
        VulkanCommandBuffer m_commandWrappers[PK_VK_MAX_COMMAND_BUFFERS]{};
        VulkanCommandBuffer* m_currentWrapper = nullptr;
    };

    struct VulkanQueueSet : public NoCopy
    {
        struct FenceSnapshot
        {
            uint64_t values[(uint32_t)QueueType::EnumCount]{};
            uint64_t index = 0ull;
        };

        constexpr static const uint32_t MAX_FENCE_SNAPSHOTS = PK_VK_MAX_GRAPH_SCOPES * 4u;

        VulkanQueueSet(const VkDevice device, const VulkanQueueSetInitializer& initializer);
        constexpr uint32_t GetQueueCount() const { return m_queueCount; }
        constexpr const VulkanQueueFamilies& GetFamiliesAll() const { return m_familiesAll; }
        constexpr const VulkanQueueFamilies& GetFamiliesRender() const { return m_familiesRender; }
        constexpr uint32_t GetQueueIndex(QueueType type) const { return m_indices[(uint32_t)type]; }
        inline VulkanQueue* GetQueue(QueueType type) { return m_queues[GetQueueIndex(type)].get(); }
        inline VulkanQueue* GetQueueAt(uint32_t index) const { return m_queues[index].get(); }
        inline VulkanQueue* GetQueueAt(uint32_t index) { return m_queues[index].get(); }

        void SetDescriptorHeaps(const VkBindHeapInfoEXT& resourceHeap, const VkBindHeapInfoEXT& samplerHeap);
        FenceRef GetFenceRef() const;
        void Prune();

    protected:
        const VkDevice m_device;
        const uint32_t m_queueCount;
        const VulkanQueueFamilies m_familiesAll;
        const VulkanQueueFamilies m_familiesRender;
        uint32_t m_indices[(uint32_t)QueueType::EnumCount]{};
        Unique<VulkanQueue> m_queues[(uint32_t)QueueType::EnumCount]{};

        mutable FenceSnapshot m_fenceSnapshots[MAX_FENCE_SNAPSHOTS]{};
        mutable uint64_t m_fenceCounter = 0ull;
    };
}
