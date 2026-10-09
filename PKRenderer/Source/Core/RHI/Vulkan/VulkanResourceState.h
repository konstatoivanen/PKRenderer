#include "Core/Base/Containers/Mask.h"
#include "Core/Base/Containers/HashMap.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanAccessRecord
    {
        union
        {
            struct
            {
                VkDeviceSize offset;
                VkDeviceSize size;
            }
            buffer;
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
        uint32_t queue;
        bool hasLayout;
    };

    struct VulkanResourceState : public NoCopy
    {
        constexpr const static uint32_t MAX_NODES = 8192u;
        constexpr const static uint32_t MAX_RESOURCES = 8192u;
        constexpr const static uint32_t MAX_PASSES = 512u;
        constexpr const static uint32_t MAX_BUFFER_BARRIERS = 2048u;
        constexpr const static uint32_t MAX_IMAGE_BARRIERS = 2048u;
        
        struct State
        {
            VkPipelineStageFlags2 stage;
            VkAccessFlags2 access;
            uint16_t hasLayout;
            uint16_t lastPass;
            uint32_t queue;
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
                buffer;
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
        
        struct ResourceHead
        {
            VkImageAspectFlags aspectMask;
            uint32_t head;
        };
        
        struct PassBarriers
        {
            VkBufferMemoryBarrier2* bufferBarriers;
            VkImageMemoryBarrier2* imageBarriers;
            uint32_t bufferBarrierCount;
            uint32_t imageBarrierCount;
            uint32_t targetPass;
        };
        
        void ResetPasses();
        uint64_t AddPass(bool coalesce);
        
        void RegisterBuffer(VkBuffer buffer);
        void RegisterImage(VkImage image, VkImageAspectFlags aspectMask);
        void UnregisterResource(uint64_t resource);

        void RecordBufferAccess(VkBuffer buffer, const VulkanAccessRecord& record);
        bool RecordImageAccess(VkImage image, const VulkanAccessRecord& record);
        void GetPassBarriers(uint32_t pass, uint32_t* outBufferBarrierCount, const VkBufferMemoryBarrier2** outBufferBarriers, uint32_t* outImageBarrierCount, const VkImageMemoryBarrier2** outImageBarriers);

    private:
        void EmitBufferBarrier(VkBuffer buffer, const State& src, const State& dst, VkDeviceSize beg, VkDeviceSize end);
        void EmitImageBarrier(VkImage image, VkImageAspectFlags aspect, const State& src, const State& dst, uint32_t layer_beg, uint32_t layer_end, uint32_t level_beg, uint32_t level_end);

        uint32_t AllocNode();
        void ListInsertBefore(uint32_t* head, uint32_t ref, uint32_t node);
        void ListInsertAfter(uint32_t ref, uint32_t node);
        void ListRemove(uint32_t* head, uint32_t node);

        Node m_nodes[MAX_NODES];
        FixedMask<MAX_NODES> m_nodeResidency;
        FixedMap<uint64_t, ResourceHead, MAX_RESOURCES> m_resources;
        VkBufferMemoryBarrier2 m_bufferBarriers[MAX_BUFFER_BARRIERS];
        VkImageMemoryBarrier2 m_imageBarriers[MAX_IMAGE_BARRIERS];
        uint32_t m_bufferBarrierHead;
        uint32_t m_imageBarrierHead;;
        PassBarriers m_passes[MAX_PASSES];
        uint64_t m_passCount;
        uint64_t m_passCounter;
        uint32_t m_coalescePass;
        bool m_isCoalescing;
    };
}