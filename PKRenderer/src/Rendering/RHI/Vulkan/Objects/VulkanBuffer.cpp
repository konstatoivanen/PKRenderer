#include "PrecompiledHeader.h"
#include "Rendering/RHI/Vulkan/VulkanDriver.h"
#include "VulkanBuffer.h"

namespace PK::Rendering::RHI::Vulkan::Objects
{
    using namespace PK::Rendering::RHI::Vulkan::Services;

    VulkanBuffer::VulkanBuffer(const BufferLayout& layout, size_t count, BufferUsage usage, const char* name) :
        Buffer(layout, count, usage),
        m_driver(RHI::Driver::GetNative<VulkanDriver>()),
        m_name(name)
    {
        Rebuild(count);
    }

    void* VulkanBuffer::BeginWrite(size_t offset, size_t size)
    {
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
        m_mapRange.ringOffset = (m_mapRange.ringOffset + m_rawBuffer->capacity) % m_mappedBuffer->capacity;

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
        m_rawBuffer->Invalidate(offset, size);
        return m_rawBuffer->BeginMap(offset);
    }

    void VulkanBuffer::EndRead()
    {
        m_rawBuffer->EndMap(0ull, 0ull);
    }


    const VulkanBindHandle* VulkanBuffer::GetBindHandle(const IndexRange& range)
    {
        PK_THROW_ASSERT(range.offset + range.count <= m_count, "Trying to get a buffer bind handle for a range that it outside of buffer bounds");

        auto index = 0u;

        if (!m_bindHandles.AddKey(range, &index))
        {
            return m_bindHandles.GetValueAt(index);
        }

        auto stride = m_layout.GetStride(m_usage);
        auto handle = m_bindHandles.GetValueAtRef(index);
        *handle = m_driver->bindhandlePool.New();
        (*handle)->buffer.buffer = m_rawBuffer->buffer;
        (*handle)->buffer.range = stride * range.count;
        (*handle)->buffer.offset = stride * range.offset;
        (*handle)->buffer.layout = &m_layout;
        (*handle)->buffer.inputRate = EnumConvert::GetInputRate(m_inputRate);
        (*handle)->isConcurrent = IsConcurrent();
        return *handle;
    }


    IndexRange VulkanBuffer::AllocateAligned(const size_t size, QueueType type)
    {
        PK_THROW_ASSERT(m_pageTable, "Non sparse buffer cannot be allocated from!");
        return m_pageTable->AllocateAligned(size, type);
    }

    void VulkanBuffer::MakeRangeResident(const IndexRange& range, QueueType type)
    {
        if (m_pageTable != nullptr)
        {
            m_pageTable->AllocateRange(range, type);
        }
    }

    void VulkanBuffer::MakeRangeNonResident(const IndexRange& range)
    {
        if (m_pageTable != nullptr)
        {
            m_pageTable->FreeRange(range);
        }
    }


    bool VulkanBuffer::Validate(size_t count)
    {
        if (m_count >= count)
        {
            return false;
        }

        Rebuild(count);
        return true;
    }

    void VulkanBuffer::Rebuild(size_t count)
    {
        // Sparse buffers cannot be persistently mapped
        if ((m_usage & BufferUsage::Sparse) != 0)
        {
            m_usage = m_usage & ~((uint32_t)BufferUsage::PersistentStage);
        }

        Dispose();

        auto size = m_layout.GetStride(m_usage) * count;
        auto bufferCreateInfo = VulkanBufferCreateInfo(m_usage, size, &m_driver->queues->GetSelectedFamilies());
        m_rawBuffer = m_driver->bufferPool.New(m_driver->device, m_driver->allocator, bufferCreateInfo, m_name.c_str());
        m_count = count;

        if ((m_usage & BufferUsage::PersistentStage) != 0)
        {
            m_mapRange.ringOffset = 0ull;
            m_mappedBuffer = m_driver->stagingBufferCache->Acquire(size, true, m_name.c_str());
        }

        if ((m_usage & BufferUsage::Sparse) != 0)
        {
            m_pageTable = new VulkanSparsePageTable(m_driver, m_rawBuffer->buffer, bufferCreateInfo.allocation.usage);
        }

        GetBindHandle({ 0, m_count });
    }

    void VulkanBuffer::Dispose()
    {
        auto fence = m_driver->GetQueues()->GetFenceRef(QueueType::Graphics);
        auto values = m_bindHandles.GetValues();

        for (auto i = 0; i < values.count; ++i)
        {
            m_driver->bindhandlePool.Delete(values[i]);
        }

        if (m_mappedBuffer != nullptr && !m_mappedBuffer->persistentmap)
        {
            m_mappedBuffer->EndMap(0, m_mapRange.region.size);
        }

        m_bindHandles.Clear();
        m_driver->disposer->Dispose(m_pageTable, fence);
        m_driver->DisposePooledBuffer(m_rawBuffer, fence);
        m_driver->stagingBufferCache->Release(m_mappedBuffer, fence);
        m_pageTable = nullptr;
        m_rawBuffer = nullptr;
        m_mappedBuffer = nullptr;
    }
}