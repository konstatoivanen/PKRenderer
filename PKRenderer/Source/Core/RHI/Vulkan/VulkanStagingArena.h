#pragma once
#include "Core/Base/NoCopy.h"
#include "Core/Base/Containers/Mask.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanStagingBuffer : public RHIBuffer
    {
        size_t GetOffset() const final { return srcOffset; }
        size_t GetSize() const final { return size; }
        BufferUsage GetUsage() const final { return BufferUsage::DefaultStaging | BufferUsage::InstanceInput; }
        const char* GetDebugName() const final { return "VulkanStagingArena"; }
        void* GetNativeHandle() const final { return buffer; }
        uint64_t GetDeviceAddress() const final { return deviceAddress + srcOffset; }
        virtual void* BeginMap([[maybe_unused]] size_t offset, [[maybe_unused]] size_t readsize) const final { return mappedData; }
        virtual void EndMap([[maybe_unused]] size_t offset, [[maybe_unused]] size_t writeSize) const final {};

        VkBuffer buffer;
        VkDeviceAddress deviceAddress;
        VkDeviceSize srcOffset;
        VkDeviceSize size;
        void* mappedData;
    };

    struct VulkanStagingArena : public NoCopy
    {
        constexpr const static uint32_t MAX_RANGES = 4096u;
        constexpr const static uint32_t MAX_STACK = 32u;

        struct BufferRange
        {
            VkDeviceSize offset;
            VkDeviceSize size;
        };

        VulkanStagingArena(VkDevice device, VmaAllocator allocator, uint64_t stagingSize);
        ~VulkanStagingArena();

        VulkanStagingBuffer* BeginWrite(VkDeviceSize size);
        VulkanStagingBuffer* EndWrite(RHIBuffer* alias);

        void BeginRange();
        uint64_t EndRange();
        void FreeRange(uint64_t rangeIndex);

    private:
        const VmaAllocator m_allocator;
        const VkDeviceSize m_size;
        VmaAllocation m_memory;
        VkDeviceAddress m_deviceAddress;
        VkBuffer m_buffer;
        void* m_mappedData;

        FixedMask<MAX_RANGES> m_rangeMask;
        BufferRange m_ranges[MAX_RANGES];

        FixedMask<MAX_STACK> m_bufferMask;
        VulkanStagingBuffer m_buffers[MAX_STACK];

        uint64_t m_rangeHead = 0ull;
        uint64_t m_bufferHead = 0ull;
        uint64_t m_bufferFlushHead = 0ull;
        uint64_t m_rangeFlushHead = 0ull;
        bool m_inRange = false;
    };
}
