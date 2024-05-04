#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/FastMap.h"
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/Vulkan/VulkanCommon.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Vulkan::Objects, class VulkanSparsePageTable)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Vulkan::Services, struct VulkanStagingBuffer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Vulkan, struct VulkanSparsePageTable)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Vulkan, struct VulkanDriver)

namespace PK::Rendering::RHI::Vulkan::Objects
{
    class VulkanBuffer : public RHI::Objects::Buffer
    {
        public:
            VulkanBuffer(size_t size, BufferUsage usage, const char* name);
            ~VulkanBuffer() { Dispose(); }

            void* BeginWrite(size_t offset, size_t size);
            void EndWrite(VkBuffer* src, VkBuffer* dst, VkBufferCopy* region, const PK::Utilities::FenceRef& fence);
            const void* BeginRead(size_t offset, size_t size) final;
            void EndRead() final;

            size_t GetCapacity() const final { return m_rawBuffer->capacity; }
            const VulkanRawBuffer* GetRaw() const { return m_rawBuffer; }
            const VulkanBindHandle* GetBindHandle(const IndexRange& range);
            inline const VulkanBindHandle* GetBindHandle() const { return m_bindHandles.GetValueAt(0); } // Default range is always the first one

            size_t SparseAllocate(const size_t size, QueueType type) final;
            void SparseAllocateRange(const IndexRange& range, QueueType type) final;
            void SparseDeallocate(const IndexRange& range) final;

            bool Validate(size_t size) final;

        private:
            struct MapRange
            {
                size_t ringOffset = 0ull;
                VkBufferCopy region = { 0ull, 0ull, 0ull };
            };

            using RangeHash = PK::Utilities::Hash::TMurmurHash<IndexRange>;

            void Rebuild(size_t size);
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