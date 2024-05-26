#pragma once
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    class VulkanBindArray : public RHIBindArray<RHITexture>,
                            public RHIBindArray<RHIBuffer>
    {
        public:
            VulkanBindArray(size_t capacity);
            ~VulkanBindArray();
            const VulkanBindHandle* const* GetHandles(uint32_t* version, uint32_t* count) const;

            int32_t Add(RHITexture* value, void* bindInfo) final;
            int32_t Add(RHITexture* value) final;
            int32_t Add(RHIBuffer* value, void* bindInfo) final;
            int32_t Add(RHIBuffer* value) final;

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