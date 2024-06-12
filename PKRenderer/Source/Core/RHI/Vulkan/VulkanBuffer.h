#pragma once
#include "Core/Utilities/FastMap.h"
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
            
            size_t GetSize() const final { return m_rawBuffer->size; }
            BufferUsage GetUsage() const final { return m_usage; }
            const char* GetDebugName() const final { return m_name.c_str(); }

            const void* BeginRead(size_t offset, size_t size) final;
            void EndRead() final;
            size_t SparseAllocate(const size_t size, QueueType type) final;
            void SparseAllocateRange(const BufferIndexRange& range, QueueType type) final;
            void SparseDeallocate(const BufferIndexRange& range) final;

            void* BeginWrite(size_t offset, size_t size);
            void EndWrite(VkBuffer* src, VkBuffer* dst, VkBufferCopy* region, const FenceRef& fence);

            const VulkanRawBuffer* GetRaw() const { return m_rawBuffer; }
            const VulkanBindHandle* GetBindHandle(const BufferIndexRange& range);
            inline const VulkanBindHandle* GetBindHandle() const { return m_bindHandles.GetValueAt(0); } // Default range is always the first one
            
        private:
            struct MapRange
            {
                size_t ringOffset = 0ull;
                VkBufferCopy region = { 0ull, 0ull, 0ull };
            };

            using RangeHash = Hash::TMurmurHash<BufferIndexRange>;

            const VulkanDriver* m_driver = nullptr;
            BufferUsage m_usage = BufferUsage::None;
            std::string m_name = "Buffer";
            VulkanRawBuffer* m_rawBuffer = nullptr;
            VulkanSparsePageTable* m_pageTable = nullptr;
            PointerMap<BufferIndexRange, VulkanBindHandle, RangeHash> m_bindHandles;
            
            //@TODO Should not reside in buffer
            VulkanStagingBuffer* m_mappedBuffer = nullptr;
            MapRange m_mapRange{};
    };
}