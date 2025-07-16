#pragma once
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    class VulkanSparsePageTable;
    struct VulkanStagingBuffer;
    class VulkanSparsePageTable;
    struct VulkanDriver;

    class VulkanBuffer : public RHIBuffer
    {
        public:
            VulkanBuffer(size_t size, BufferUsage usage, const char* name);
            ~VulkanBuffer();
            
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

            // @TODO bad pattern. Difficult to inline into begin/end map as user wont know the necessary flush context.
            void* BeginStagedWrite(size_t offset, size_t size);
            void EndStagedWrite(RHIBuffer** dst, RHIBuffer** src, VkBufferCopy* region, const FenceRef& fence);

            constexpr const VulkanBindHandle* GetBindHandle() const { return m_defaultView; }
            const VulkanBindHandle* GetBindHandle(const BufferIndexRange& range);
            
        private:
            const VulkanDriver* m_driver = nullptr;
            BufferUsage m_usage = BufferUsage::None;
            FixedString128 m_name;
            VulkanRawBuffer* m_buffer = nullptr;
            VulkanStagingBuffer* m_stage = nullptr;
            VulkanSparsePageTable* m_pageTable = nullptr;
            VulkanBufferView* m_defaultView = nullptr;
            FastLinkedListRoot<VulkanBufferView, BufferIndexRange> m_firstView = nullptr;

            struct
            {
                VkDeviceSize ringOffset = 0u;
                VkDeviceSize srcOffset = 0u;
                VkDeviceSize dstOffset = 0u;
                VkDeviceSize size = 0u;
            }
            m_stageRegion{};
    };
}