#pragma once
#include "Core/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Utilities;
    using namespace Systems;

    class VulkanBuffer : public PK::Core::NoCopy
    {
        public:
            VulkanBuffer(const VulkanDriver* driver, BufferUsage usage, const BufferLayout& layout, size_t count);
            ~VulkanBuffer();

            void SetData(const void* data, size_t offset, size_t size) const;
            bool Validate(size_t count);

            constexpr const BufferUsage GetUsage() const { return m_usage; }
            constexpr const BufferLayout& GetLayout() const { return m_layout; }
            size_t GetCapacity() const { return m_rawBuffer->capacity; }

            const VulkanBindHandle* GetBindHandle() const;

        private:
            void Rebuild(size_t count);
            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            Ref<VulkanRawBuffer> m_rawBuffer = nullptr;
            Scope<VulkanBindHandle> m_bindHandle = nullptr;
            BufferLayout m_layout{};
            BufferUsage m_usage = BufferUsage::None;
            size_t m_count = 0;
    };
}