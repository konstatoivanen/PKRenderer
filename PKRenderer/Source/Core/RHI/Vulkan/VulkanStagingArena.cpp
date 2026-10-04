#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanStagingArena.h"

namespace PK
{
    VulkanStagingArena::VulkanStagingArena(const VulkanDriver* driver, uint64_t stagingSize) :
        m_allocator(driver->allocator),
        m_size(math::align(stagingSize, 512ull))
    {
        if (m_size)
        {
            const auto& queueFamilies = driver->queues->GetSelectedFamilies();

            // @TODO unify with VulkanRawBuffer once that has been updated.
            VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferCreateInfo.size = m_size;
            bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferCreateInfo.queueFamilyIndexCount = queueFamilies.count;
            bufferCreateInfo.pQueueFamilyIndices = queueFamilies.indices;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | 
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            
            VmaAllocationInfo allocationInfo{};
            VK_ASSERT_RESULT(vmaCreateBuffer(m_allocator, &bufferCreateInfo, &allocationCreateInfo, &m_buffer, &m_memory, &allocationInfo));
            m_mappedData = allocationInfo.pMappedData;
            
            VulkanSetObjectDebugName(driver->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, "VulkanStagingArena");

            VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
            addressInfo.buffer = m_buffer;
            m_deviceAddress = vkGetBufferDeviceAddress(driver->device, &addressInfo);
        }
    }

    VulkanStagingArena::~VulkanStagingArena()
    {
        if (m_size)
        {
            vmaDestroyBuffer(m_allocator, m_buffer, m_memory);
        }
    }

    VulkanStagingBuffer* VulkanStagingArena::BeginWrite(VkDeviceSize size)
    {
        auto index = m_bufferMask.FindFirstZero();

        PK_DEBUG_FATAL_ASSERT(m_inRange && m_size && size && index >= 0 && index < MAX_STACK, "VulkanStagingArena: Failed to acquire staging buffer!");

        const auto alignedSize = math::align(size, 64ull); // 512
        auto allocationOffset = m_bufferHead % m_size;

        if (allocationOffset + alignedSize > m_size)
        {
            m_bufferHead += (m_size - allocationOffset);
            allocationOffset = 0ull;
        }
        
        PK_DEBUG_FATAL_ASSERT(m_bufferHead - m_bufferFlushHead + alignedSize <= m_size, "VulkanStagingArena: overflow!");
        m_bufferHead += alignedSize;

        auto buffer = &m_buffers[index];
        m_bufferMask[index] = true;
        buffer->handle.buffer.buffer = m_buffer;
        buffer->handle.buffer.deviceAddress = m_deviceAddress + allocationOffset;
        buffer->handle.buffer.offset = allocationOffset;
        buffer->handle.buffer.range = size;
        buffer->mappedData = static_cast<uint8_t*>(m_mappedData) + allocationOffset;
        return buffer;
    }

    VulkanStagingBuffer* VulkanStagingArena::EndWrite(RHIBuffer* alias)
    {
        auto buffer = static_cast<VulkanStagingBuffer*>(alias);
        auto buffers = &m_buffers[0];
        auto index = (uint32_t)(buffer - buffers);
        PK_DEBUG_FATAL_ASSERT(buffer >= buffers && buffer < buffers + MAX_STACK, "VulkanStagingArena: Trying to end write scope outside of pool bounds!");
        m_bufferMask[index] = false;
        return buffer;
    }

    void VulkanStagingArena::BeginRange()
    {
        PK_DEBUG_FATAL_ASSERT(!m_inRange && m_rangeHead - m_rangeFlushHead < MAX_RANGES, "VulkanStagingArena: Failed to begin allocation range!");
        m_inRange = true;
        auto& range = m_ranges[m_rangeHead % MAX_RANGES];
        range.offset = m_bufferHead;
        range.size = 0ull;
    }

    uint64_t VulkanStagingArena::EndRange()
    {
        PK_DEBUG_FATAL_ASSERT(m_inRange, "VulkanStagingArena: Failed to end allocation range!");
        PK_DEBUG_FATAL_ASSERT(m_bufferMask.CountBits() == 0, "VulkanStagingArena: out of execution scope writes!");
        auto index = m_rangeHead;
        auto& range = m_ranges[m_rangeHead % MAX_RANGES];
        range.size = m_bufferHead - range.offset;
        m_inRange = false;
        ++m_rangeHead;
        return index;
    }

    void VulkanStagingArena::FreeRange(uint64_t rangeIndex)
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
