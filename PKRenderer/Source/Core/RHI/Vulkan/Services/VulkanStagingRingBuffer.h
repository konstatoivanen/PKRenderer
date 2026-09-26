#pragma once
#include "Core/Base/NoCopy.h"
#include "Core/Base/Containers/Mask.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanStagingScope : public RHIBuffer
    {
        VkBuffer buffer;
        VkDeviceAddress deviceAddress;
        void* mappedData;
        VkBufferCopy region;

        size_t GetOffset() const final { return region.srcOffset; }
        size_t GetSize() const final { return region.size; }
        BufferUsage GetUsage() const final { return BufferUsage::DefaultStaging | BufferUsage::InstanceInput; }
        const char* GetDebugName() const final { return "VulkanStagingRingBuffer"; }
        void* GetNativeHandle() const final { return buffer; }
        uint64_t GetDeviceAddress() const final { return deviceAddress + region.srcOffset; }

        size_t SparseAllocate(const size_t size, QueueType type) final { return 0ull; }
        void SparseAllocateRange(const BufferIndexRange& range, QueueType type) final {};
        void SparseDeallocate(const BufferIndexRange& range) final {};

        virtual void* BeginMap(size_t offset, size_t readsize) const final { return mappedData; }
        virtual void EndMap(size_t offset, size_t size) const final {};
    };

    struct VulkanStagingRingBuffer : public NoCopy
    {
        constexpr const static uint32_t MAX_RANGES = 4096u;
        constexpr const static uint32_t MAX_STACK = 32u;

        struct BufferRange
        {
            VkDeviceSize offset;
            VkDeviceSize size;
        };

        VulkanStagingRingBuffer(VkDevice device, VmaAllocator allocator, uint64_t stagingSize);
        ~VulkanStagingRingBuffer();

        VulkanStagingScope* BeginWrite(VkDeviceSize offset, VkDeviceSize size);
        VulkanStagingScope* EndWrite(RHIBuffer* buffer);

        void BeginRange();
        uint64_t EndRange();
        void FreeRange(uint64_t rangeIndex);

    private:
        const VmaAllocator m_allocator;
        const VkDevice m_device;
        const VkDeviceSize m_size;
        VmaAllocation m_memory;
        VkDeviceAddress m_deviceAddress;
        VkBuffer m_buffer;
        void* m_mappedData;

        FixedMask<MAX_RANGES> m_rangeMask;
        BufferRange m_ranges[MAX_RANGES];

        FixedMask<MAX_STACK> m_scopeMask;
        VulkanStagingScope m_scopes[MAX_STACK];

        uint64_t m_rangeHead = 0ull;
        uint64_t m_bufferHead = 0ull;
        uint64_t m_bufferFlushHead = 0ull;
        uint64_t m_rangeFlushHead = 0ull;
        bool m_inRange = false;
    };
}
