#pragma once
#include "Rendering/Objects/Buffer.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/Objects/BindArray.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Core;
    using namespace PK::Rendering::Objects;

    class VulkanBindArray : public BindArray<Texture>,
                            public BindArray<Buffer>
    {
        public:
            VulkanBindArray(size_t capacity);
            ~VulkanBindArray();
            const VulkanBindHandle* const* GetHandles(uint32_t* version, uint32_t* count) const;

            int32_t Add(Texture* value) override final;
            int32_t Add(const Texture* value) override final;
            int32_t Add(Buffer* value) override final;
            int32_t Add(const Buffer* value) override final;

            void Clear() override final { m_count = 0; }

        private:
            int Add(const VulkanBindHandle* handle);

            const VulkanBindHandle** m_handles = nullptr;
            // Hackedy hack dirty stuff
            mutable bool m_isDirty = false;
            mutable size_t m_previousCount = 0ull;
            mutable uint32_t m_version = 0u;
            size_t m_count = 0ull;
            size_t m_capacity = 0ull;
    };
}