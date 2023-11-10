#pragma once
#include "Utilities/FastMap.h"
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/Vulkan/Objects/VulkanSparsePageTable.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanStructs.h"
#include "Rendering/RHI/Vulkan/VulkanDriver.h"

namespace PK::Rendering::RHI::Vulkan::Objects
{
    class VulkanBuffer : public RHI::Objects::Buffer
    {
        public:
            VulkanBuffer(const BufferLayout& layout, size_t count, BufferUsage usage, const char* name);
            ~VulkanBuffer() { Dispose(); }

            void* BeginWrite(size_t offset, size_t size);
            void EndWrite(VkBuffer* src, VkBuffer* dst, VkBufferCopy* region, const FenceRef& fence);
            const void* BeginRead(size_t offset, size_t size) final;
            void EndRead() final;

            size_t GetCapacity() const final { return m_rawBuffer->capacity; }
            const VulkanRawBuffer* GetRaw() const { return m_rawBuffer; }
            const VulkanBindHandle* GetBindHandle(const IndexRange& range);
            inline const VulkanBindHandle* GetBindHandle() const { return m_bindHandles.GetValueAt(0); } // Default range is always the first one

            IndexRange AllocateAligned(const size_t size, QueueType type) final;
            void MakeRangeResident(const IndexRange& range, QueueType type) final;
            void MakeRangeNonResident(const IndexRange& range) final;

            bool Validate(size_t count) final;

        private:
            struct MapRange
            {
                size_t ringOffset = 0ull;
                VkBufferCopy region = { 0ull, 0ull, 0ull };
            };

            using RangeHash = PK::Utilities::HashHelpers::TMurmurHash<IndexRange>;

            void Rebuild(size_t count);
            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            std::string m_name = "Buffer";
            VulkanRawBuffer* m_rawBuffer = nullptr;
            Services::VulkanStagingBuffer* m_mappedBuffer = nullptr;
            VulkanSparsePageTable* m_pageTable = nullptr;
            MapRange m_mapRange{};
            PK::Utilities::PointerMap<IndexRange, VulkanBindHandle, RangeHash> m_bindHandles;
    };
}