#include "PrecompiledHeader.h"
#include "VulkanBuffer.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace Services;

    VulkanBuffer::VulkanBuffer(const BufferLayout& layout, size_t count, BufferUsage usage, const char* name) :
        Buffer(layout, count, usage),
        m_driver(GraphicsAPI::GetActiveDriver<VulkanDriver>()),
        m_name(name)
    {
        Rebuild(count);
    }

    VulkanBuffer::~VulkanBuffer()
    {
        Dispose(m_driver->GetCommandBuffer(QueueType::Graphics)->GetOnCompleteGate());

        // Borrowed staging buffer not returned :/
        if (m_mappedBuffer != nullptr)
        {
            m_mappedBuffer->EndMap(0, m_mapRange.region.size);
            m_mappedBuffer = nullptr;
        }
    }

    void* VulkanBuffer::BeginWrite(size_t offset, size_t size)
    {
        m_mapRange.region.dstOffset = offset;
        m_mapRange.region.size = size;

        if ((m_usage & BufferUsage::PersistentStage) == 0)
        {
            PK_THROW_ASSERT(m_mappedBuffer == nullptr, "Trying to begin a new mapping for a buffer that is already being mapped!");
            m_mappedBuffer = m_driver->stagingBufferCache->GetBuffer(size, m_driver->GetCommandBuffer(QueueType::Graphics)->GetOnCompleteGate());
            m_mapRange.region.srcOffset = 0ull;
            return m_mappedBuffer->BeginMap(0ull);
        }
        
        // Local persistent stage
        m_mapRange.region.srcOffset = m_mapRange.ringOffset + offset;
        return m_mappedBuffer->BeginMap(m_mapRange.region.srcOffset);
    }

    void VulkanBuffer::EndWrite(VkBuffer* src, VkBuffer* dst, VkBufferCopy* region)
    {
        PK_THROW_ASSERT(m_mappedBuffer != nullptr, "Trying to end buffer map for an unmapped buffer!");
        
        m_mappedBuffer->EndMap(m_mapRange.region.srcOffset, m_mapRange.region.size);
        m_mapRange.ringOffset = (m_mapRange.ringOffset + m_rawBuffer->capacity) % m_mappedBuffer->capacity;
        
        *src = m_mappedBuffer->buffer;
        *dst = m_rawBuffer->buffer;
        *region = m_mapRange.region;

        if ((m_usage & BufferUsage::PersistentStage) == 0)
        {
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

        VulkanBindHandle* handle = nullptr;

        if (m_bindHandles.TryGetValue(range, &handle))
        {
            return handle;
        }

        handle = new VulkanBindHandle();
        auto stride = m_layout.GetStride(m_usage);
        handle->buffer.buffer = m_rawBuffer->buffer;
        handle->buffer.range = stride * range.count;
        handle->buffer.offset = stride * range.offset;
        handle->buffer.layout = &m_layout;
        handle->buffer.inputRate = EnumConvert::GetInputRate(m_inputRate);
        m_bindHandles.AddValue(range, handle);
        return handle;
    }


    void VulkanBuffer::MakeRangeResident(const IndexRange& range)
    {
        if (m_pageTable != nullptr)
        {
            m_pageTable->AllocateRange(range);
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

        Dispose(m_driver->GetCommandBuffer(QueueType::Graphics)->GetOnCompleteGate());

        m_count = count;
        auto size = m_layout.GetStride(m_usage) * count;
        auto bufferCreateInfo = VulkanBufferCreateInfo(m_usage, size);
        m_rawBuffer = new VulkanRawBuffer(m_driver->device, m_driver->allocator, bufferCreateInfo, m_name.c_str());

        if ((m_usage & BufferUsage::PersistentStage) != 0)
        {
            m_mapRange.ringOffset = 0ull;
            m_mappedBuffer = new VulkanStagingBuffer(m_driver->device,
                                                     m_driver->allocator, 
                                                     VulkanBufferCreateInfo(BufferUsage::DefaultStaging | BufferUsage::PersistentStage, size * PK_MAX_FRAMES_IN_FLIGHT),
                                                     (std::string(m_name) + std::string(".StagingBuffer")).c_str());
        }

        if ((m_usage & BufferUsage::Sparse) != 0)
        {
            m_pageTable = new VulkanSparsePageTable(m_driver, m_rawBuffer->buffer, bufferCreateInfo.allocation.usage);
        }

        GetBindHandle({ 0, m_count });
    }

    void VulkanBuffer::Dispose(const ExecutionGate& gate)
    {
        auto values = m_bindHandles.GetValues();

        for (auto i = 0; i < values.count; ++i)
        {
            delete values[i];
        }

        m_bindHandles.Clear();

        if (m_pageTable != nullptr)
        {
            m_driver->disposer->Dispose(m_pageTable, gate);
            m_pageTable = nullptr;
        }

        if (m_rawBuffer != nullptr)
        {
            m_driver->disposer->Dispose(m_rawBuffer, gate);
            m_rawBuffer = nullptr;
        }

        if (m_mappedBuffer != nullptr && (m_usage & BufferUsage::PersistentStage) != 0)
        {
            m_driver->disposer->Dispose(m_mappedBuffer, gate);
            m_mappedBuffer = nullptr;
        }
    }
}