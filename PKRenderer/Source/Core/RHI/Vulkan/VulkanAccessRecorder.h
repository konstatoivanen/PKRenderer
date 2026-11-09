#pragma once
#include "Core/Base/Containers/Mask.h"
#include "Core/Base/Containers/HashMap.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"

namespace PK
{
    struct VulkanAccessRecord
    {
        union
        {
            VkDeviceAddressRangeKHR memory;

            struct
            {
                uint32_t baseMipLevel;
                uint32_t levelCount;
                uint32_t baseArrayLayer;
                uint32_t layerCount;
            }
            image;
        };

        VkPipelineStageFlags2 stage;
        VkAccessFlags2 access;
        uint8_t queue;
        uint16_t isLocked;
    };

    struct VulkanPassDependencies
    {
        VkMemoryBarrier2* memoryBarrier = nullptr;
        const VkImageMemoryBarrier2* imageTransitions = nullptr;
        const uint32_t* waitPasses = nullptr;
        VkPipelineStageFlags2 waitStageMask = 0ull;
        uint32_t imageTransitionCount = 0u;
    };

    struct VulkanAccessRecorder : public NoCopy
    {
        struct State
        {
            VkPipelineStageFlags2 writeStage;
            VkAccessFlags2 writeAccess;
            uint32_t writePass;
            uint8_t writeQueue;
            uint8_t isLocked;
            uint16_t readMask;
            VkPipelineStageFlags2 readStage[(uint32_t)QueueType::EnumCount];
            uint32_t readPass[(uint32_t)QueueType::EnumCount];
        };
        
        struct Node
        {
            union
            {
                struct
                {
                    VkDeviceSize offset;
                    VkDeviceSize size;
                }
                memory;
                struct
                {
                    uint32_t baseMipLevel;
                    uint32_t levelCount;
                    uint32_t baseArrayLayer;
                    uint32_t layerCount;
                }
                image;
            };

            State state;
            uint32_t next;
            uint32_t prev;
        };
        
        struct ImageHead
        {
            VkImageAspectFlags aspectMask : 16;
            uint32_t isNew : 16;
            uint32_t head;
        };
        
        struct PassBarriers
        {
            VkImageMemoryBarrier2* imageTransitions;
            uint32_t imageTransitionCount;
            VkMemoryBarrier2 memoryBarrier;
            VkPipelineStageFlags2 waitStageMask;
            uint32_t waitPasses[(uint32_t)QueueType::EnumCount];
        };

        constexpr uint64_t GetPassCounter() const { return m_passCounter; }
        constexpr uint64_t GetPassFirst() const { return m_passFirst; }
        constexpr uint64_t GetPassFlush() const { return m_passFlush; }
        constexpr uint64_t GetPassCount() const { return m_passCount; }
        constexpr const uint32_t* GetResidencyWaitPasses() const { return m_residencyWaitPasses; }
        
        void ResetPasses(uint64_t flushedPassIndex);
        uint64_t AddPass();
        
        void RegisterImage(VkImage image, VkImageAspectFlags aspectMask);
        void UnregisterImage(VkImage image);

        void RecordMemoryAccess(const VulkanAccessRecord& record);
        void RecordImageAccess(VkImage image, const VulkanAccessRecord& record);
        
        VulkanPassDependencies GetPassDependencies(uint32_t passIndex);

    private:
        void ResolveAccess(State& state, const VulkanAccessRecord& record);
        void EmitBarrier(uint32_t srcQueue, uint32_t srcPass, VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess);
        void EmitWait(uint32_t srcQueue, uint32_t srcPass, VkPipelineStageFlags2 dstStage);

        uint32_t AllocNode();
        void ListInsertBefore(uint32_t* head, uint32_t ref, uint32_t node);
        void ListInsertAfter(uint32_t ref, uint32_t node);
        void ListRemove(uint32_t* head, uint32_t node);

        Node m_nodes[PK_VK_MAX_ACCESS_NODES];
        FixedMask<PK_VK_MAX_ACCESS_NODES> m_nodeResidency;
        FixedMap<uint64_t, ImageHead, PK_VK_MAX_ACCESS_RESOURCES> m_images;
        PassBarriers m_passes[PK_VK_MAX_ACCESS_PASSES];
        VkImageMemoryBarrier2 m_imageTransitions[PK_VK_MAX_ACCESS_IMAGE_BARRIERS];
        uint32_t m_residencyWaitPasses[(uint32_t)QueueType::EnumCount]{};
        uint32_t m_memoryHead = ~0u;
        uint64_t m_imageTransitionHead = 0ull;
        uint64_t m_passCount = 0ull;
        uint64_t m_passCounter = 0ull;
        uint64_t m_passFlush = 0ull;
        uint64_t m_passFirst = 0ull;
    };
}