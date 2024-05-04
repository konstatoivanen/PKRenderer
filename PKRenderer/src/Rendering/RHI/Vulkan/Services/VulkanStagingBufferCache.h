#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/FixedPool.h"
#include "Rendering/RHI/Vulkan/VulkanCommon.h"

namespace PK::Rendering::RHI::Vulkan::Services
{
    struct VulkanStagingBuffer : public VulkanRawBuffer
    {
        VulkanStagingBuffer(VkDevice device, VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo, const char* name) : VulkanRawBuffer(device, allocator, createInfo, name) {}
        mutable FenceRef fence;
        uint64_t pruneTick = 0ull;
    };

    class VulkanStagingBufferCache : public PK::Utilities::NoCopy
    {
        public:
            VulkanStagingBufferCache(VkDevice device, VmaAllocator allocator, uint64_t pruneDelay);
            ~VulkanStagingBufferCache();
            VulkanStagingBuffer* Acquire(size_t size, bool persistent, const char* name);
            void Release(VulkanStagingBuffer* buffer, const FenceRef& fence);
            void Prune();

        private:
            const VmaAllocator m_allocator;
            const VkDevice m_device;
            std::vector<VulkanStagingBuffer*> m_freeBuffers;
            std::vector<VulkanStagingBuffer*> m_activeBuffers;
            PK::Utilities::FixedPool<VulkanStagingBuffer, 1024> m_bufferPool;
            uint64_t m_currentPruneTick = 0ull;
            uint64_t m_pruneDelay = 0ull;
    };
}