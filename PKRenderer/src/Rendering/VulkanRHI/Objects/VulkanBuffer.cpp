#include "PrecompiledHeader.h"
#include "VulkanBuffer.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace Systems;

    VulkanBuffer::VulkanBuffer(const BufferLayout& layout, const void* data, size_t count, BufferUsage usage, const char* name) :
        Buffer(layout, count, usage),
        m_driver(GraphicsAPI::GetActiveDriver<VulkanDriver>()),
        m_name(name)
    {
        Rebuild(count);
        
        if (data != nullptr)
        {
            SetData(data, 0, m_layout.GetStride(m_usage) * count);
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
        Dispose();

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
            m_mappedBuffer = m_driver->stagingBufferCache->GetBuffer(size, m_driver->commandBufferPool->GetCurrent()->GetOnCompleteGate());
            m_mapRange.region.srcOffset = 0ull;
            return m_mappedBuffer->BeginMap(0ull);
        }
        
        m_mapRange.region.srcOffset = m_mapRange.ringOffset + offset;

        // Local persistent stage
        return m_mappedBuffer->BeginMap(m_mapRange.region.srcOffset);
    }

    void VulkanBuffer::EndWrite()
    {
        PK_THROW_ASSERT(m_mappedBuffer != nullptr, "Trying to end buffer map for an unmapped buffer!");
        
        auto* cmd = m_driver->commandBufferPool->GetCurrent();
        auto persistent = (m_usage & BufferUsage::PersistentStage) != 0;
        
        m_mappedBuffer->EndMap(m_mapRange.region.srcOffset, m_mapRange.region.size);
        cmd->CopyBuffer(m_mappedBuffer->buffer, m_rawBuffer->buffer, 1, &m_mapRange.region);

        VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = m_rawBuffer->buffer;
        barrier.offset = m_mapRange.region.dstOffset;
        barrier.size = m_mapRange.region.size;

        if (!persistent)
        {
            m_mappedBuffer = nullptr;
        }
        else
        {
            m_mapRange.ringOffset = (m_mapRange.ringOffset + m_rawBuffer->capacity) % m_mappedBuffer->capacity;
        }

        uint32_t dstStageMask = 0u;

        if (m_rawBuffer->usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT || m_rawBuffer->usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        {
            barrier.dstAccessMask |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
            dstStageMask |= VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        }

        if (m_rawBuffer->usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            barrier.dstAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
            dstStageMask |= VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        }

        if (m_rawBuffer->usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
            barrier.dstAccessMask |= VK_ACCESS_MEMORY_READ_BIT;
            dstStageMask |= VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        }

        if (m_rawBuffer->usage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
        {
            barrier.dstAccessMask |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            dstStageMask |= VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        }

        if (dstStageMask != 0)
        {
            cmd->PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, dstStageMask, 0, 0, nullptr, 1, &barrier, 0, nullptr);
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
        handle->buffer = m_rawBuffer->buffer;
        handle->bufferRange = stride * range.count;
        handle->bufferOffset = stride * range.offset;
        handle->bufferLayout = &m_layout;
        handle->inputRate = EnumConvert::GetInputRate(m_inputRate);
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

        Dispose();

        m_count = count;
        auto size = m_layout.GetStride(m_usage) * count;
        auto bufferCreateInfo = VulkanBufferCreateInfo(m_usage, size);
        m_rawBuffer = new VulkanRawBuffer(m_driver->device, m_driver->allocator, bufferCreateInfo, m_name.c_str());

        if ((m_usage & BufferUsage::PersistentStage) != 0)
        {
            auto stagingName = std::string(m_name) + std::string(" (Staging Buffer)");
            m_mappedBuffer = new VulkanStagingBuffer(m_driver->device,
                                                     m_driver->allocator, 
                                                     VulkanBufferCreateInfo(BufferUsage::DefaultStaging | BufferUsage::PersistentStage, size * PK_MAX_FRAMES_IN_FLIGHT),
                                                     stagingName.c_str());
        }

        if ((m_usage & BufferUsage::Sparse) != 0)
        {
            m_pageTable = new VulkanSparsePageTable(m_driver, m_rawBuffer->buffer, bufferCreateInfo.allocation.usage);
        }

        GetBindHandle({ 0, m_count });
    }

    void VulkanBuffer::Dispose()
    {
        auto values = m_bindHandles.GetValues();

        for (auto i = 0; i < values.count; ++i)
        {
            delete values[i];
        }

        m_bindHandles.Clear();

        if (m_pageTable != nullptr)
        {
            m_driver->disposer->Dispose(m_pageTable, m_driver->commandBufferPool->GetCurrent()->GetOnCompleteGate());
            m_pageTable = nullptr;
        }

        if (m_rawBuffer != nullptr)
        {
            m_driver->disposer->Dispose(m_rawBuffer, m_driver->commandBufferPool->GetCurrent()->GetOnCompleteGate());
            m_rawBuffer = nullptr;
        }

        if (m_mappedBuffer != nullptr && (m_usage & BufferUsage::PersistentStage) != 0)
        {
            m_driver->disposer->Dispose(m_mappedBuffer, m_driver->commandBufferPool->GetCurrent()->GetOnCompleteGate());
            m_mappedBuffer = nullptr;
        }
    }
}