#pragma once
#include "Rendering/Objects/Buffer.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/VulkanRHI/Objects/VulkanSparsePageTable.h"
#include "Utilities/FastMap.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    class VulkanBuffer : public Buffer
    {
        public:
            VulkanBuffer(const Structs::BufferLayout& layout, size_t count, Structs::BufferUsage usage, const char* name);
            ~VulkanBuffer();

            void* BeginWrite(const Structs::FenceRef& fence, size_t offset, size_t size);
            void EndWrite(VkBuffer* src, VkBuffer* dst, VkBufferCopy* region);
            const void* BeginRead(size_t offset, size_t size) final;
            void EndRead() final;

            size_t GetCapacity() const final { return m_rawBuffer->capacity; }
            const VulkanRawBuffer* GetRaw() const { return m_rawBuffer; }
            const VulkanBindHandle* GetBindHandle(const Structs::IndexRange& range);
            // Default range is always the first one
            inline const VulkanBindHandle* GetBindHandle() const { return m_bindHandles.GetValueAt(0); }

            void MakeRangeResident(const Structs::IndexRange& range, QueueType type) final;
            void MakeRangeNonResident(const Structs::IndexRange& range) final;

            bool Validate(size_t count) final;

        private:
            struct MapRange
            {
                size_t ringOffset = 0ull;
                VkBufferCopy region = { 0ull, 0ull, 0ull };
            };

            using RangeHash = PK::Utilities::HashHelpers::TMurmurHash<Structs::IndexRange>;

            void Rebuild(size_t count);
            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            std::string m_name = "Buffer";
            VulkanRawBuffer* m_rawBuffer = nullptr;
            Services::VulkanStagingBuffer* m_mappedBuffer = nullptr;
            VulkanSparsePageTable* m_pageTable = nullptr;
            MapRange m_mapRange{};
            PK::Utilities::PointerMap<Structs::IndexRange, VulkanBindHandle, RangeHash> m_bindHandles;
    };
}