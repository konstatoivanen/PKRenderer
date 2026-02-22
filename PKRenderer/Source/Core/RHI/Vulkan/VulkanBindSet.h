#pragma once
#include "Core/Utilities/FastMap.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    class VulkanBindSet : public RHIBindSet<RHITexture>,
                            public RHIBindSet<RHIBuffer>
    {
        public:
            VulkanBindSet(size_t capacity);

            const VulkanBindHandle* const* GetHandles(uint32_t* version, uint32_t* count) const;

            int32_t Add(RHITexture* value, void* bindInfo) final;
            int32_t Add(RHITexture* value) final;
            int32_t Add(RHIBuffer* value, void* bindInfo) final;
            int32_t Add(RHIBuffer* value) final;
            uint3 GetBoundTextureSize(uint32_t index) const final;
            BufferIndexRange GetBoundBufferRange(uint32_t index) const final;

            void Clear() final { m_set.ClearFast(); }

        private:
            int32_t Add(const VulkanBindHandle* handle);

            FastSet16<const VulkanBindHandle*, Hash::TPointerHash<VulkanBindHandle>> m_set;

            // Hackedy hack dirty stuff
            mutable bool m_isDirty = false;
            mutable size_t m_previousCount = 0ull;
            mutable uint32_t m_version = 0u;
    };
}
