#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "VulkanStagingRingBuffer.h"

namespace PK
{
    VulkanStagingRingBuffer::VulkanStagingRingBuffer(VkDevice device, VmaAllocator allocator, uint64_t stagingSize) :
        m_allocator(allocator),
        m_device(device),
        m_size(math::align(stagingSize, 512ull))
    {
        if (m_size)
        {
            // @TODO unify with VulkanRawBuffer once that has been updated.
            VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferCreateInfo.size = m_size;
            bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | 
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            
            VmaAllocationInfo allocationInfo{};
            VK_ASSERT_RESULT(vmaCreateBuffer(m_allocator, &bufferCreateInfo, &allocationCreateInfo, &m_buffer, &m_memory, &allocationInfo));
            m_mappedData = allocationInfo.pMappedData;
            
            VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, "VulkanStagingRingBuffer");

            VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
            addressInfo.buffer = m_buffer;
            m_deviceAddress = vkGetBufferDeviceAddress(device, &addressInfo);
        }
    }

    VulkanStagingRingBuffer::~VulkanStagingRingBuffer()
    {
        vmaDestroyBuffer(m_allocator, m_buffer, m_memory);
    }

    VulkanStagingBuffer* VulkanStagingRingBuffer::BeginWrite(VkDeviceSize size)
    {
        auto index = m_bufferMask.FindFirstZero();

        PK_FATAL_ASSERT(m_inRange && m_size && size && index >= 0 && index < MAX_STACK, "VulkanStagingRingBuffer: Failed to acquire staging buffer!");

        const auto alignedSize = math::align(size, 512ull);
        auto allocationOffset = m_bufferHead % m_size;

        if (allocationOffset + alignedSize > m_size)
        {
            m_bufferHead += (m_size - allocationOffset);
            allocationOffset = 0ull;
        }
        
        PK_FATAL_ASSERT(m_bufferHead - m_bufferFlushHead + alignedSize <= m_size, "VulkanStagingRingBuffer: overflow!");
        m_bufferHead += alignedSize;

        auto buffer = &m_buffers[index];
        m_bufferMask[index] = true;
        buffer->buffer = m_buffer;
        buffer->deviceAddress = m_deviceAddress;
        buffer->mappedData = static_cast<uint8_t*>(m_mappedData) + allocationOffset;
        buffer->srcOffset = allocationOffset;
        buffer->size = size;
        return buffer;
    }

    VulkanStagingBuffer* VulkanStagingRingBuffer::EndWrite(RHIBuffer* alias)
    {
        auto buffer = static_cast<VulkanStagingBuffer*>(alias);
        auto buffers = &m_buffers[0];
        auto index = (uint32_t)(buffer - buffers);
        PK_FATAL_ASSERT(buffer >= buffers && buffer < buffers + MAX_STACK, "VulkanStagingRingBuffer: Trying to end write scope outside of pool bounds!");
        m_bufferMask[index] = false;
        return buffer;
    }

    void VulkanStagingRingBuffer::BeginRange()
    {
        PK_FATAL_ASSERT(!m_inRange && m_rangeHead - m_rangeFlushHead < MAX_RANGES, "VulkanStagingRingBuffer: Failed to begin allocation range!");
        m_inRange = true;
        auto& range = m_ranges[m_rangeHead % MAX_RANGES];
        range.offset = m_bufferHead;
        range.size = 0ull;
    }

    uint64_t VulkanStagingRingBuffer::EndRange()
    {
        PK_FATAL_ASSERT(m_inRange, "VulkanStagingRingBuffer: Failed to end allocation range!");
        PK_FATAL_ASSERT(m_bufferMask.CountBits() == 0, "VulkanStagingRingBuffer: out of execution scope writes!");
        auto index = m_rangeHead;
        auto& range = m_ranges[m_rangeHead % MAX_RANGES];
        range.size = m_bufferHead - range.offset;
        m_inRange = false;
        ++m_rangeHead;
        return index;
    }

    void VulkanStagingRingBuffer::FreeRange(uint64_t rangeIndex)
    {
        if (rangeIndex >= m_rangeFlushHead)
        {
            m_rangeMask[rangeIndex % MAX_RANGES] = true;

            // Increment out of order counters
            if (rangeIndex == m_rangeFlushHead)
            {
                while (m_rangeMask[m_rangeFlushHead % MAX_RANGES])
                {
                    m_rangeMask[m_rangeFlushHead % MAX_RANGES] = false;
                    m_bufferFlushHead += m_ranges[m_rangeFlushHead % MAX_RANGES].size;
                    ++m_rangeFlushHead;
                }
            }
        }
    }
}
