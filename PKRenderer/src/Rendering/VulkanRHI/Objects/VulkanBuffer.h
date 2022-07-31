#pragma once
#include "Rendering/Objects/Buffer.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/VulkanRHI/Objects/VulkanSparsePageTable.h"
#include "Utilities/PointerMap.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    class VulkanBuffer : public Buffer
    {
        public:
            VulkanBuffer(const Structs::BufferLayout& layout, const void* data, size_t count, Structs::BufferUsage usage, const char* name);
            ~VulkanBuffer();

            void SetData(const void* data, size_t offset, size_t size) override final;
            void SetSubData(const void* data, size_t offset, size_t size) override final;
            void* BeginWrite(size_t offset, size_t size) override final;
            void EndWrite() override final;
            const void* BeginRead(size_t offset, size_t size) override final;
            void EndRead() override final;

            size_t GetCapacity() const override final { return m_rawBuffer->capacity; }
            const VulkanRawBuffer* GetRaw() const { return m_rawBuffer; }
            const VulkanBindHandle* GetBindHandle(const Structs::IndexRange& range);
            inline const VulkanBindHandle* GetBindHandle() const
            {
                // Default range is always the first one
                return m_bindHandles.GetValueAt(0);
            }

            void MakeRangeResident(const Structs::IndexRange& range) override final;
            void MakeRangeNonResident(const Structs::IndexRange& range)  override final;

            bool Validate(size_t count) override final;

        private:
            struct RangeHash
            {
                size_t operator()(const Structs::IndexRange& k) const noexcept
                {
                    return Utilities::HashHelpers::FNV1AHash(&k, sizeof(Structs::IndexRange));
                }
            };

            void Rebuild(size_t count);
            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            std::string m_name = "Buffer";
            VulkanRawBuffer* m_rawBuffer = nullptr;
            Systems::VulkanStagingBuffer* m_mappedBuffer = nullptr;
            VulkanSparsePageTable* m_pageTable = nullptr;
            Utilities::PointerMap<Structs::IndexRange, VulkanBindHandle, RangeHash> m_bindHandles;
    };
}