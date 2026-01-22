#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/Disposer.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanStagingBuffer : public VulkanRawBuffer, public RHIBuffer
    {
        VulkanStagingBuffer(VkDevice device, VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo, const char* name) : VulkanRawBuffer(device, allocator, createInfo, name) {}

        size_t GetSize() const final { return size; }
        BufferUsage GetUsage() const final { return BufferUsage::DefaultStaging; }
        const char* GetDebugName() const final { return nullptr; }
        void* GetNativeHandle() const final { return buffer; }
        uint64_t GetDeviceAddress() const { return deviceAddress; }

        void* BeginMap(size_t offset, size_t readsize) const final { return VulkanRawBuffer::BeginMap(offset, readsize); }
        void EndMap(size_t offset, size_t size) const final { VulkanRawBuffer::EndMap(offset, size); }

        size_t SparseAllocate([[maybe_unused]] const size_t size, [[maybe_unused]] QueueType type) final { return 0ull; }
        void SparseAllocateRange([[maybe_unused]] const BufferIndexRange& range, [[maybe_unused]] QueueType type) final {}
        void SparseDeallocate([[maybe_unused]] const BufferIndexRange& range) final {}

        FenceRef fence;
        uint64_t pruneTick = 0ull;
        VulkanStagingBuffer* next = nullptr;
    };

    class VulkanStagingBufferCache : public NoCopy
    {
        public:
            VulkanStagingBufferCache(Disposer* disposer, VkDevice device, VmaAllocator allocator, uint64_t pruneDelay);
            ~VulkanStagingBufferCache();

            VulkanStagingBuffer* Acquire(size_t size, bool persistent, const char* name);
            void Release(VulkanStagingBuffer* buffer, const FenceRef& fence);
            void Prune();

        private:

            const VmaAllocator m_allocator;
            const VkDevice m_device;
            Disposer* m_disposer;
            VulkanStagingBuffer* m_freeBufferHead = nullptr;
            VulkanStagingBuffer* m_liveBufferHead = nullptr;
            FixedPool<VulkanStagingBuffer, 512> m_bufferPool;
            uint64_t m_currentPruneTick = 0ull;
            uint64_t m_pruneDelay = 0ull;
    };
}
