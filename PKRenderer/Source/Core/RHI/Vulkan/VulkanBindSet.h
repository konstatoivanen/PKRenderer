#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanBindSet : public RHIBindSet<RHITexture>,
                           public RHIBindSet<RHIBuffer>
    {
        VulkanBindSet(size_t capacity);

        const VulkanResourceView* GetViews(uint64_t* version, uint32_t* count) const;

        int32_t Add(RHITexture* value, void* bindInfo) final;
        int32_t Add(RHITexture* value) final;
        int32_t Add(RHIBuffer* value, void* bindInfo) final;
        int32_t Add(RHIBuffer* value) final;
        uint3 GetBoundTextureSize(uint32_t index) const final;

        void Clear() final { m_set.ClearFast(); }

    private:
        int32_t Add(const VulkanResourceView& handle);

        HashSet16<VulkanResourceView, VulkanResourceViewHash> m_set;

        // Hackedy hack dirty stuff
        mutable bool m_isDirty = false;
        mutable size_t m_previousCount = 0ull;
        mutable uint64_t m_version = 0ull;
    };
}
