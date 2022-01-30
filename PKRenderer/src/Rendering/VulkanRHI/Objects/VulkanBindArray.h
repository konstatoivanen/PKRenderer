#pragma once
#include "Rendering/Objects/Buffer.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/Objects/BindArray.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    class VulkanBindArray : public Rendering::Objects::BindArray<Rendering::Objects::Texture>,
                            public Rendering::Objects::BindArray<Rendering::Objects::Buffer>
    {
        public:
            VulkanBindArray(size_t capacity);
            ~VulkanBindArray();
            const VulkanBindHandle* const* GetHandles(uint32_t* version, uint32_t* count) const;

            int32_t Add(Rendering::Objects::Texture* value, void* bindInfo) override final;
            int32_t Add(Rendering::Objects::Texture* value) override final;
            int32_t Add(const Rendering::Objects::Texture* value) override final;
            int32_t Add(Rendering::Objects::Buffer* value, void* bindInfo) override final;
            int32_t Add(Rendering::Objects::Buffer* value) override final;
            int32_t Add(const Rendering::Objects::Buffer* value) override final;

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