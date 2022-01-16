#pragma once
#include "Rendering/Objects/Buffer.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Utilities/PointerMap.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Rendering::Objects;
    using namespace PK::Utilities;
    using namespace Systems;

    class VulkanBuffer : public Buffer
    {
        public:
            VulkanBuffer(BufferUsage usage, const BufferLayout& layout, const void* data, size_t count);
            ~VulkanBuffer();

            void SetData(const void* data, size_t offset, size_t size) override final;
            void* BeginMap(size_t offset, size_t size) override final;
            void EndMap() override final;

            bool Validate(size_t count) override final;

            size_t GetCapacity() const override final { return m_rawBuffer->capacity; }
            const VulkanRawBuffer* GetRaw() const { return m_rawBuffer; }
            const VulkanBindHandle* GetBindHandle(const IndexRange& range);
            inline const VulkanBindHandle* GetBindHandle() const
            {
                // Default range is always the first one
                return m_bindHandles.GetValueAt(0);
            }

        private:
            struct RangeHash
            {
                size_t operator()(const IndexRange& k) const noexcept
                {
                    return HashHelpers::FNV1AHash(&k, sizeof(IndexRange));
                }
            };

            void Rebuild(size_t count);
            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            VulkanRawBuffer* m_rawBuffer = nullptr;
            VulkanStagingBuffer* m_mappedBuffer = nullptr;
            PointerMap<IndexRange, VulkanBindHandle, RangeHash> m_bindHandles;
    };
}