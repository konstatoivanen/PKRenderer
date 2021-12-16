#pragma once
#include "Rendering/Objects/Buffer.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Rendering::Objects;
    using namespace PK::Utilities;
    using namespace Systems;

    class VulkanBuffer : public Buffer
    {
        public:
            VulkanBuffer(BufferUsage usage, const BufferLayout& layout, size_t count);
            VulkanBuffer(BufferUsage usage, const BufferLayout& layout, const void* data, size_t count);
            ~VulkanBuffer();

            void SetData(const void* data, size_t offset, size_t size) override final;
            void* BeginMap(size_t offset, size_t size) override final;
            void EndMap() override final;

            bool Validate(size_t count) override final;

            size_t GetCapacity() const override final { return m_rawBuffer->capacity; }
            const VulkanRawBuffer* GetRaw() const { return m_rawBuffer; }
            const VulkanBindHandle* GetBindHandle() const;

        private:
            void Rebuild(size_t count);
            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            VulkanRawBuffer* m_rawBuffer = nullptr;
            Scope<VulkanBindHandle> m_bindHandle = nullptr;
            const VulkanStagingBuffer* m_mappedBuffer = nullptr;
            uint32_t m_version = 0u;
    };
}