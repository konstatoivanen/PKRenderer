#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/FixedPool.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanStagingBuffer : public VulkanRawBuffer
    {
        VulkanStagingBuffer(VkDevice device, VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo, const char* name) : VulkanRawBuffer(device, allocator, createInfo, name) {}
        FenceRef fence;
        uint64_t pruneTick = 0ull;
        VulkanStagingBuffer* next = nullptr;
    };

    class VulkanStagingBufferCache : public NoCopy
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
            VulkanStagingBuffer* m_freeBufferHead = nullptr;
            VulkanStagingBuffer* m_liveBufferHead = nullptr;
            FixedPool<VulkanStagingBuffer, 1024> m_bufferPool;
            uint64_t m_currentPruneTick = 0ull;
            uint64_t m_pruneDelay = 0ull;
    };
}
