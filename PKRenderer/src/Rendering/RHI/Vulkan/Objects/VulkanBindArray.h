#pragma once
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/Objects/Texture.h"
#include "Rendering/RHI/Objects/BindArray.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanStructs.h"
#include "Rendering/RHI/Vulkan/VulkanDriver.h"

namespace PK::Rendering::RHI::Vulkan::Objects
{
    class VulkanBindArray : public RHI::Objects::BindArray<RHI::Objects::Texture>,
                            public RHI::Objects::BindArray<RHI::Objects::Buffer>
    {
        public:
            VulkanBindArray(size_t capacity);
            ~VulkanBindArray();
            const VulkanBindHandle* const* GetHandles(uint32_t* version, uint32_t* count) const;

            int32_t Add(RHI::Objects::Texture* value, void* bindInfo) final;
            int32_t Add(RHI::Objects::Texture* value) final;
            int32_t Add(RHI::Objects::Buffer* value, void* bindInfo) final;
            int32_t Add(RHI::Objects::Buffer* value) final;

            void Clear() final { m_count = 0; }

        private:
            int32_t Add(const VulkanBindHandle* handle);

            const VulkanBindHandle** m_handles = nullptr;
            // Hackedy hack dirty stuff
            mutable bool m_isDirty = false;
            mutable size_t m_previousCount = 0ull;
            mutable uint32_t m_version = 0u;
            size_t m_count = 0ull;
            size_t m_capacity = 0ull;
    };
}