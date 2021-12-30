#include "PrecompiledHeader.h"
#include "VulkanBuffer.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    VulkanBuffer::VulkanBuffer(BufferUsage usage, const BufferLayout& layout, const void* data, size_t count) :
        Buffer(usage, layout, count),
        m_driver(GraphicsAPI::GetActiveDriver<VulkanDriver>())
    {
        Rebuild(count);
        
        if (data != nullptr)
        {
            SetData(data, 0, m_layout.GetStride(m_usage) * count);
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
        if (m_mappedBuffer != nullptr)
        {
            m_mappedBuffer->EndMap(0, m_mappedBuffer->desitnationRange);
            m_mappedBuffer = nullptr;
        }

        Dispose();
    }

    void VulkanBuffer::SetData(const void* data, size_t offset, size_t size)
    {
        auto dst = BeginMap(0, GetCapacity());
        memcpy(reinterpret_cast<char*>(dst) + offset, data, size);
        EndMap();
    }

    void* VulkanBuffer::BeginMap(size_t offset, size_t size)
    {
        m_mappedBuffer = m_driver->stagingBufferCache->GetBuffer(size);
        m_mappedBuffer->executionGate = m_driver->commandBufferPool->GetCurrent()->GetOnCompleteGate();
        m_mappedBuffer->destinationOffset = offset;
        m_mappedBuffer->desitnationRange = size;
        return m_mappedBuffer->BeginMap(0ull);
    }

    void VulkanBuffer::EndMap()
    {
        PK_THROW_ASSERT(m_mappedBuffer != nullptr, "Trying to end buffer map for an unmapped buffer!");
        
        auto* cmd = m_driver->commandBufferPool->GetCurrent();

        VkBufferCopy region{};
        region.srcOffset = 0;
        region.dstOffset = m_mappedBuffer->destinationOffset;
        region.size = m_mappedBuffer->desitnationRange;

        m_mappedBuffer->EndMap(0, region.size);
        cmd->CopyBuffer(m_mappedBuffer->buffer, m_rawBuffer->buffer, 1, &region);
        m_mappedBuffer = nullptr;

        VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = m_rawBuffer->buffer;
        barrier.size = VK_WHOLE_SIZE;

        if (m_rawBuffer->usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT || m_rawBuffer->usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        {
            barrier.dstAccessMask |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
            cmd->PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                0, 0, nullptr, 1, &barrier, 0, nullptr);
        }

        if (m_rawBuffer->usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            barrier.dstAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
            cmd->PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                0, 0, nullptr, 1, &barrier, 0, nullptr);
        }

        if (m_rawBuffer->usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
            barrier.dstAccessMask |= VK_ACCESS_MEMORY_READ_BIT;
            cmd->PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                0, 0, nullptr, 1, &barrier, 0, nullptr);
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

    const VulkanBindHandle* VulkanBuffer::GetBindHandle(const IndexRange& range)
    {
        PK_THROW_ASSERT(range.offset + range.count <= m_count, "Trying to get a buffer bind handle for a range that it outside of buffer bounds");

        auto iter = m_bindHandles.find(range);

        if (iter != m_bindHandles.end())
        {
            return iter->second.get();
        }

        auto newHandle = new VulkanBindHandle();
        auto stride = m_layout.GetStride(m_usage);
        newHandle->version = m_version;
        newHandle->buffer = m_rawBuffer->buffer;
        newHandle->bufferRange = stride * range.count;
        newHandle->bufferOffset = stride * range.offset;
        newHandle->bufferLayout = &m_layout;
        newHandle->inputRate = EnumConvert::GetInputRate(m_inputRate);
        m_bindHandles[range] = Scope<VulkanBindHandle>(newHandle);
        return newHandle;
    }

    void VulkanBuffer::Rebuild(size_t count)
    {
        Dispose();
        m_version++;
        m_count = count;
        m_rawBuffer = new VulkanRawBuffer(m_driver->allocator, VulkanBufferCreateInfo(m_usage, m_layout.GetStride(m_usage) * count));
        GetBindHandle({ 0, m_count });
    }

    void VulkanBuffer::Dispose()
    {
        m_bindHandles.clear();

        if (m_rawBuffer != nullptr)
        {
            m_driver->disposer->Dispose(m_rawBuffer, m_driver->commandBufferPool->GetCurrent()->GetOnCompleteGate());
            m_rawBuffer = nullptr;
        }
    }
}