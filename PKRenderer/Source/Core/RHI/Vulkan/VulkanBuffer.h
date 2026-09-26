#pragma once
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanBuffer : public RHIBuffer
    {
        VulkanBuffer(struct VulkanDriver* driver, size_t size, BufferUsage usage, const char* name);
        ~VulkanBuffer();
        
        size_t GetOffset() const final { return 0ull; }
        size_t GetSize() const final { return m_buffer->size; }
        BufferUsage GetUsage() const final { return m_usage; }
        const char* GetDebugName() const final { return m_name.c_str(); }
        void* GetNativeHandle() const final { return m_buffer->buffer; }
        uint64_t GetDeviceAddress() const final { return m_buffer->deviceAddress; }

        void* BeginMap(size_t offset, size_t readsize) const final;
        void EndMap(size_t offset, size_t size) const final;

        size_t SparseAllocate(const size_t size, QueueType type) final;
        void SparseAllocateRange(const BufferIndexRange& range, QueueType type) final;
        void SparseDeallocate(const BufferIndexRange& range) final;

        constexpr const VulkanBindHandle* GetBindHandle() const { return m_defaultView; }
        const VulkanBindHandle* GetBindHandle(const BufferIndexRange& range);
            
    private:
        const VulkanDriver* m_driver;
        const FixedString128 m_name;
        BufferUsage m_usage = BufferUsage::None;
        VulkanRawBuffer* m_buffer = nullptr;
        struct VulkanSparsePageTable* m_pageTable = nullptr;
        VulkanBufferView* m_defaultView = nullptr;
        LinkedList<VulkanBufferView, BufferIndexRange> m_firstView = nullptr;
    };
}
