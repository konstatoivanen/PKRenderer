#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Utilities/FixedString.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "Core/RHI/Vulkan/VulkanSparsePageTable.h"
#include "VulkanBuffer.h"

namespace PK
{
    VulkanBuffer::VulkanBuffer(size_t size, BufferUsage usage, const char* name) :
        m_driver(RHIDriver::Get()->GetNative<VulkanDriver>()),
        m_usage(usage),
        m_name(name)
    {
        // Sparse buffers cannot be persistently mapped
        if ((m_usage & BufferUsage::Sparse) != 0)
        {
            m_usage = m_usage & ~((uint32_t)BufferUsage::PersistentStage);
        }

        auto bufferCreateInfo = VulkanBufferCreateInfo(m_usage, size, &m_driver->queues->GetSelectedFamilies());
        m_rawBuffer = m_driver->bufferPool.New(m_driver->device, m_driver->allocator, bufferCreateInfo, m_name.c_str());

        if ((m_usage & BufferUsage::PersistentStage) != 0)
        {
            m_mapRange.ringOffset = 0ull;
            m_mappedBuffer = m_driver->stagingBufferCache->Acquire(size, true, m_name.c_str());
        }

        if ((m_usage & BufferUsage::Sparse) != 0)
        {
            FixedString128 pageTableName({ name, ".PageTable" });
            m_pageTable = new VulkanSparsePageTable(m_driver, m_rawBuffer->buffer, bufferCreateInfo.allocation.usage, pageTableName.c_str());
        }

        GetBindHandle({ 0, GetSize() });
        m_defaultView = m_firstView;
    }

    VulkanBuffer::~VulkanBuffer()
    {
        auto fence = m_driver->GetQueues()->GetFenceRef(QueueType::Graphics);

        for (auto view : m_firstView)
        {
            m_driver->bufferViewPool.Delete(view);
        }

        if (m_mappedBuffer != nullptr && !m_mappedBuffer->persistentmap)
        {
            m_mappedBuffer->EndMap(0, m_mapRange.region.size);
        }

        m_driver->disposer->Dispose(m_pageTable, fence);
        m_driver->DisposePooledBuffer(m_rawBuffer, fence);
        m_driver->stagingBufferCache->Release(m_mappedBuffer, fence);
        m_pageTable = nullptr;
        m_rawBuffer = nullptr;
        m_mappedBuffer = nullptr;
        m_firstView = nullptr;
        m_defaultView = nullptr;
    }

    void* VulkanBuffer::BeginWrite(size_t offset, size_t size)
    {
        PK_THROW_ASSERT((offset + size) <= GetSize(), "Map buffer range exceeds buffer bounds, map size: %i, buffer size: %i", offset + size, GetSize());

        m_mapRange.region.dstOffset = offset;
        m_mapRange.region.size = size;

        if ((m_usage & BufferUsage::PersistentStage) == 0)
        {
            PK_THROW_ASSERT(m_mappedBuffer == nullptr, "Trying to begin a new mapping for a buffer that is already being mapped!");
            m_mappedBuffer = m_driver->stagingBufferCache->Acquire(size, false, nullptr);
            m_mapRange.region.srcOffset = 0ull;
            return m_mappedBuffer->BeginMap(0ull);
        }

        // Local persistent stage
        m_mapRange.region.srcOffset = m_mapRange.ringOffset + offset;
        return m_mappedBuffer->BeginMap(m_mapRange.region.srcOffset);
    }

    void VulkanBuffer::EndWrite(VkBuffer* src, VkBuffer* dst, VkBufferCopy* region, const FenceRef& fence)
    {
        PK_THROW_ASSERT(m_mappedBuffer != nullptr, "Trying to end buffer map for an unmapped buffer!");

        m_mappedBuffer->EndMap(m_mapRange.region.srcOffset, m_mapRange.region.size);
        m_mapRange.ringOffset = (m_mapRange.ringOffset + m_rawBuffer->size) % m_mappedBuffer->size;

        *src = m_mappedBuffer->buffer;
        *dst = m_rawBuffer->buffer;
        *region = m_mapRange.region;

        if ((m_usage & BufferUsage::PersistentStage) == 0)
        {
            m_driver->stagingBufferCache->Release(m_mappedBuffer, fence);
            m_mappedBuffer = nullptr;
        }
    }

    const void* VulkanBuffer::BeginRead(size_t offset, size_t size)
    {
        PK_THROW_ASSERT((offset + size) <= GetSize(), "Map buffer range exceeds buffer bounds, map size: %i, buffer size: %i", offset + size, GetSize());
        m_rawBuffer->Invalidate(offset, size);
        return m_rawBuffer->BeginMap(offset);
    }

    void VulkanBuffer::EndRead()
    {
        m_rawBuffer->EndMap(0ull, 0ull);
    }


    const VulkanBindHandle* VulkanBuffer::GetBindHandle(const BufferIndexRange& range)
    {
        PK_THROW_ASSERT(range.offset + range.count <= GetSize(), "Trying to get a buffer bind handle for a range that it outside of buffer bounds");

        if (m_firstView.FindAndSwapFirst(range))
        {
            return m_firstView;
        }

        auto view = m_driver->bufferViewPool.New();
        view->buffer.buffer = m_rawBuffer->buffer;
        view->buffer.range = range.count;
        view->buffer.offset = range.offset;
        view->isConcurrent = IsConcurrent();
        m_firstView.Insert(view, range);
        return m_firstView;
    }


    size_t VulkanBuffer::SparseAllocate(const size_t size, QueueType type)
    {
        PK_THROW_ASSERT(m_pageTable, "Non sparse buffer cannot be allocated from!");
        return m_pageTable->Allocate(size, type);
    }

    void VulkanBuffer::SparseAllocateRange(const BufferIndexRange& range, QueueType type)
    {
        PK_THROW_ASSERT(m_pageTable, "Non sparse buffer cannot be allocated from!");
        m_pageTable->AllocateRange(range, type);
    }

    void VulkanBuffer::SparseDeallocate(const BufferIndexRange& range)
    {
        PK_THROW_ASSERT(m_pageTable, "Non sparse buffer cannot be deallocated from!");
        m_pageTable->DeallocateRange(range);
    }
}