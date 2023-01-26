#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"

namespace PK::Rendering::VulkanRHI::Services
{
    struct VulkanBarrierInfo
    {
        VkPipelineStageFlags srcStageMask;
        VkPipelineStageFlags dstStageMask;
        VkDependencyFlags dependencyFlags;
        uint32_t bufferMemoryBarrierCount;
        const VkBufferMemoryBarrier* pBufferMemoryBarriers;
        uint32_t imageMemoryBarrierCount;
        const VkImageMemoryBarrier* pImageMemoryBarriers;
    };

    class VulkanBarrierHandler : PK::Utilities::NoCopy
    {
        struct BufferAccess
        {
            uint32_t offset = 0u;
            uint32_t size = 0u;
            VkPipelineStageFlags stage = 0u;
            VkAccessFlags access = 0u;
            uint32_t previous = 0u;
        };

        struct ImageAccess
        {
            Structs::TextureViewRange range{};
            VkPipelineStageFlags stage = 0u;
            VkAccessFlags access = 0u;
            uint32_t previous = 0u;
        };

        public:
            VulkanBarrierHandler(VkDevice device) : m_device(device) {}
            ~VulkanBarrierHandler();

            void RecordAccess(VkBuffer buffer, uint32_t offset, uint32_t size, VkPipelineStageFlags stage, VkAccessFlags access);
            void RecordAccess(VkImage image, const Structs::TextureViewRange& range, VkImageLayout layout, VkPipelineStageFlags stage, VkAccessFlags access);
            bool Resolve(VulkanBarrierInfo* outBarrierInfo);

        private:
            const VkDevice m_device;
            std::unordered_map<VkBuffer, BufferAccess> m_buffers;
            std::unordered_map<VkImage, ImageAccess> m_images;
            std::vector<BufferAccess> m_bufferAccesses;
            std::vector<ImageAccess> m_imageAccesses;
            std::vector<VkBufferMemoryBarrier> m_bufferBarriers;
            std::vector<VkImageMemoryBarrier> m_imageBarriers;
    };
}