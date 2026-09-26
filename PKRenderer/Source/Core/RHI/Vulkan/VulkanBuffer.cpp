#include "PrecompiledHeader.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "Core/RHI/Vulkan/VulkanSparsePageTable.h"
#include "Core/CLI/Log.h"
#include "VulkanBuffer.h"

namespace PK
{
    VulkanBuffer::VulkanBuffer(VulkanDriver* driver, size_t size, BufferUsage usage, const char* name) :
        m_driver(driver),
        m_name(name),
        m_usage(usage)
    {
        auto bufferCreateInfo = VulkanBufferCreateInfo(m_usage, size, &m_driver->queues->GetSelectedFamilies());
        m_buffer = m_driver->CreatePooled<VulkanRawBuffer>(m_driver->device, m_driver->allocator, bufferCreateInfo, m_name.c_str());

        if ((m_usage & BufferUsage::Sparse) != 0)
        {
            FixedString128 pageTableName({ name, ".PageTable" });
            m_pageTable = Memory::New<VulkanSparsePageTable>(m_driver, m_buffer->buffer, bufferCreateInfo.allocation.usage, pageTableName.c_str());
        }

        // host local buffers cannot be bound and dont need tracking.
        if ((m_usage & BufferUsage::TypeBits) != BufferUsage::CPUOnly)
        {
            GetBindHandle({ 0, GetSize() });
            m_defaultView = m_firstView;
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
        auto fence = m_driver->GetQueues()->GetLastSubmitFenceRef();

        for (auto view : m_firstView)
        {
            m_driver->DisposePooled(view, fence);
        }

        m_driver->disposer->Dispose(m_pageTable, fence);
        m_driver->DisposePooled(m_buffer, fence);
        m_pageTable = nullptr;
        m_buffer = nullptr;
        m_firstView = nullptr;
        m_defaultView = nullptr;
    }

    void* VulkanBuffer::BeginMap(size_t offset, size_t readsize) const
    {
        PK_DEBUG_FATAL_ASSERT((offset + readsize) <= GetSize(), "Map buffer range exceeds buffer bounds, map size: %i, buffer size: %i", offset + readsize, GetSize());
        PK_DEBUG_FATAL_ASSERT((m_usage & BufferUsage::TypeBits) != BufferUsage::GPUOnly, "Cant map a gpu only buffer");
        return m_buffer->BeginMap(offset, readsize);
    }

    void VulkanBuffer::EndMap(size_t offset, size_t size) const
    {
        m_buffer->EndMap(offset, size);
    }

    size_t VulkanBuffer::SparseAllocate(const size_t size, QueueType type)
    {
        PK_DEBUG_FATAL_ASSERT(m_pageTable, "Non sparse buffer cannot be allocated from!");
        return m_pageTable->Allocate(size, type);
    }

    void VulkanBuffer::SparseAllocateRange(const BufferIndexRange& range, QueueType type)
    {
        PK_DEBUG_FATAL_ASSERT(m_pageTable, "Non sparse buffer cannot be allocated from!");
        m_pageTable->AllocateRange(range, type);
    }

    void VulkanBuffer::SparseDeallocate(const BufferIndexRange& range)
    {
        PK_DEBUG_FATAL_ASSERT(m_pageTable, "Non sparse buffer cannot be deallocated from!");
        m_pageTable->DeallocateRange(range);
    }

    const VulkanBindHandle* VulkanBuffer::GetBindHandle(const BufferIndexRange& range)
    {
        PK_DEBUG_FATAL_ASSERT(range.offset + range.count <= GetSize(), "Trying to get a buffer bind handle for a range that it outside of buffer bounds");

        if (m_firstView.FindAndSwapFirst(range))
        {
            return m_firstView;
        }

        auto view = m_driver->CreatePooled<VulkanBufferView>();
        view->buffer.buffer = m_buffer->buffer;
        view->buffer.range = range.count;
        view->buffer.offset = range.offset;
        view->isConcurrent = IsConcurrent();
        m_firstView.Insert(view, range);
        return m_firstView;
    }
}
