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
        uint16_t queueMask;
        uint16_t isLocked;
    };

    struct VulkanPassDependencies
    {
        const VkMemoryBarrier2* memoryBarrier = nullptr;
        const VkImageMemoryBarrier2* imageTransitions = nullptr;
        uint32_t imageTransitionCount = 0u;
        uint32_t dependencyMask = 0u;
    };

    struct VulkanAccessRecorder : public NoCopy
    {
        struct State
        {
            VkPipelineStageFlags2 stage;
            VkAccessFlags2 access;
            uint32_t lastPass;
            uint16_t queueMask;
            uint16_t isLocked;
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
            uint32_t dependencyMask;
        };

        constexpr uint64_t GetPassCounter() const { return m_passCounter; }
        
        void ResetPasses(uint64_t flushedPassIndex);
        uint64_t AddPass();
        
        void RegisterImage(VkImage image, VkImageAspectFlags aspectMask);
        void UnregisterImage(VkImage image);

        void RecordMemoryAccess(const VulkanAccessRecord& record);
        void RecordImageAccess(VkImage image, const VulkanAccessRecord& record);
        
        VulkanPassDependencies GetPassDependencies(uint32_t passIndex);

    private:
        void EmitMemoryBarrier(const State& src, const State& dst);

        uint32_t AllocNode();
        void ListInsertBefore(uint32_t* head, uint32_t ref, uint32_t node);
        void ListInsertAfter(uint32_t ref, uint32_t node);
        void ListRemove(uint32_t* head, uint32_t node);

        Node m_nodes[PK_VK_MAX_ACCESS_NODES];
        FixedMask<PK_VK_MAX_ACCESS_NODES> m_nodeResidency;
        FixedMap<uint64_t, ImageHead, PK_VK_MAX_ACCESS_RESOURCES> m_images;
        PassBarriers m_passes[PK_VK_MAX_ACCESS_PASSES];
        VkImageMemoryBarrier2 m_imageTransitions[PK_VK_MAX_ACCESS_IMAGE_BARRIERS];
        uint32_t m_memoryHead = ~0u;
        uint64_t m_imageTransitionHead = 0ull;
        uint64_t m_passCount = 0ull;
        uint64_t m_passCounter = 0ull;
        uint64_t m_passFlush = 0ull;
        uint64_t m_passFirst = 0ull;
    };
}