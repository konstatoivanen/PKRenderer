#pragma once
#include "Core/NoCopy.h"
#include "Utilities/Ref.h"
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
            ~VulkanBuffer();

            void SetData(const void* data, size_t offset, size_t size) const override final;
            bool Validate(size_t count) override final;

            size_t GetCapacity() const override final { return m_rawBuffer->capacity; }

            const VulkanBindHandle* GetBindHandle() const;

        private:
            void Rebuild(size_t count);
            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            Ref<VulkanRawBuffer> m_rawBuffer = nullptr;
            Scope<VulkanBindHandle> m_bindHandle = nullptr;
    };
}