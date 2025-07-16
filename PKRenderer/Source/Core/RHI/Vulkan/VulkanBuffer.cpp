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

        // Dont persistent map the backing buffer.
        auto bufferUsage = m_usage & ~((uint32_t)BufferUsage::PersistentStage);
        auto bufferCreateInfo = VulkanBufferCreateInfo(bufferUsage, size, &m_driver->queues->GetSelectedFamilies());
        m_rawBuffer = m_driver->CreatePooled<VulkanRawBuffer>(m_driver->device, m_driver->allocator, bufferCreateInfo, m_name.c_str());

        // Acquire persistent staging buffer
        if ((m_usage & BufferUsage::PersistentStage) != 0)
        {
            m_stage = m_driver->stagingBufferCache->Acquire(size, true, m_name.c_str());
        }

        if ((m_usage & BufferUsage::Sparse) != 0)
        {
            FixedString128 pageTableName({ name, ".PageTable" });
            m_pageTable = new VulkanSparsePageTable(m_driver, m_rawBuffer->buffer, bufferCreateInfo.allocation.usage, pageTableName.c_str());
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

        if (m_stage != nullptr && !m_stage->isPersistentMap)
        {
            m_stage->EndMap(0ull, 0ull);
        }

        m_driver->stagingBufferCache->Release(m_stage, fence);
        m_stage = nullptr;

        m_driver->disposer->Dispose(m_pageTable, fence);
        m_driver->DisposePooled(m_rawBuffer, fence);
        m_pageTable = nullptr;
        m_rawBuffer = nullptr;
        m_firstView = nullptr;
        m_defaultView = nullptr;
    }



    void* VulkanBuffer::BeginMap(size_t offset, size_t readsize) const
    {
        PK_DEBUG_THROW_ASSERT((offset + readsize) <= GetSize(), "Map buffer range exceeds buffer bounds, map size: %i, buffer size: %i", offset + readsize, GetSize());
        PK_DEBUG_THROW_ASSERT((m_usage & BufferUsage::TypeBits) != BufferUsage::GPUOnly, "Cant map a gpu only buffer");
        return m_rawBuffer->BeginMap(offset, readsize);
    }

    void VulkanBuffer::EndMap(size_t offset, size_t size) const
    {
        m_rawBuffer->EndMap(offset, size);
    }

    size_t VulkanBuffer::SparseAllocate(const size_t size, QueueType type)
    {
        PK_DEBUG_THROW_ASSERT(m_pageTable, "Non sparse buffer cannot be allocated from!");
        return m_pageTable->Allocate(size, type);
    }

    void VulkanBuffer::SparseAllocateRange(const BufferIndexRange& range, QueueType type)
    {
        PK_DEBUG_THROW_ASSERT(m_pageTable, "Non sparse buffer cannot be allocated from!");
        m_pageTable->AllocateRange(range, type);
    }

    void VulkanBuffer::SparseDeallocate(const BufferIndexRange& range)
    {
        PK_DEBUG_THROW_ASSERT(m_pageTable, "Non sparse buffer cannot be deallocated from!");
        m_pageTable->DeallocateRange(range);
    }


    void* VulkanBuffer::BeginStagedWrite(size_t offset, size_t size)
    {
        PK_DEBUG_THROW_ASSERT((offset + size) <= GetSize(), "Map buffer range exceeds buffer bounds, map size: %i, buffer size: %i", offset + size, GetSize());

        if (m_stage == nullptr || !m_stage->isPersistentMap)
        {
            PK_DEBUG_THROW_ASSERT(m_stage == nullptr, "Trying to begin a new mapping for a buffer that is already being mapped!");
            m_stage = m_driver->stagingBufferCache->Acquire(size, false, nullptr);
        }

        m_stageRegion.srcOffset = m_stage->isPersistentMap ? m_stageRegion.ringOffset + offset : 0ull;
        m_stageRegion.dstOffset = offset;
        m_stageRegion.size = size;
        return m_stage->BeginMap(m_stageRegion.srcOffset, 0ull);
    }

    void VulkanBuffer::EndStagedWrite(RHIBuffer** dst, RHIBuffer** src, VkBufferCopy* region, const FenceRef& fence)
    {
        PK_DEBUG_THROW_ASSERT(m_stage != nullptr, "Trying to end buffer map for an unmapped buffer!");
        
        *src = m_stage;
        *dst = this;
        region->srcOffset = m_stageRegion.srcOffset;
        region->dstOffset = m_stageRegion.dstOffset;
        region->size = m_stageRegion.size;

        m_stage->EndMap(m_stageRegion.srcOffset, m_stageRegion.size);
        m_stageRegion.ringOffset = (m_stageRegion.ringOffset + m_rawBuffer->size) % m_stage->size;

        if (!m_stage->isPersistentMap)
        {
            m_driver->stagingBufferCache->Release(m_stage, fence);
            m_stage = nullptr;
        }
    }

    const VulkanBindHandle* VulkanBuffer::GetBindHandle(const BufferIndexRange& range)
    {
        PK_DEBUG_THROW_ASSERT(range.offset + range.count <= GetSize(), "Trying to get a buffer bind handle for a range that it outside of buffer bounds");

        if (m_firstView.FindAndSwapFirst(range))
        {
            return m_firstView;
        }

        auto view = m_driver->CreatePooled<VulkanBufferView>();
        view->buffer.buffer = m_rawBuffer->buffer;
        view->buffer.range = range.count;
        view->buffer.offset = range.offset;
        view->isConcurrent = IsConcurrent();
        m_firstView.Insert(view, range);
        return m_firstView;
    }
}
