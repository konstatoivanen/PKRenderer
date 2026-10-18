#include "PrecompiledHeader.h"
#include "Core/RHI/Vulkan/VulkanTexture.h"
#include "Core/RHI/Vulkan/VulkanBuffer.h"
#include "VulkanBindSet.h"

namespace PK
{
    VulkanBindSet::VulkanBindSet(size_t capacity) : m_set((uint32_t)capacity, 1u)
    {
    }

    const VulkanResourceView* VulkanBindSet::GetViews(uint64_t* version, uint32_t* count) const
    {
        if (m_set.GetCount() != m_previousCount)
        {
            m_previousCount = m_set.GetCount();
            m_isDirty = true;
        }

        if (m_isDirty)
        {
            ++m_version;
            m_isDirty = false;
        }

        *count = m_set.GetCount();
        *version = m_version;
        return m_set.GetValues();
    }

    int32_t VulkanBindSet::Add(RHITexture* value, void* bindInfo)
    {
        if (bindInfo)
        {
            return Add(*value->GetNativeView<VulkanResourceView>(*static_cast<TextureViewRange*>(bindInfo), TextureViewMode::SRV));
        }

        return Add(value);
    }

    int32_t VulkanBindSet::Add(RHITexture* value)
    {
        return Add(*value->GetNativeView<VulkanResourceView>({}, TextureViewMode::SRV));
    }

    int32_t VulkanBindSet::Add(RHIBuffer* value, void* bindInfo)
    {
        if (bindInfo)
        {
            auto vkBuffer = static_cast<VulkanBuffer*>(value);
            return Add(vkBuffer->GetResourceView(*static_cast<BufferIndexRange*>(bindInfo)));
        }

        return Add(value);
    }

    int32_t VulkanBindSet::Add(RHIBuffer* value)
    {
        return Add(*value->GetNativeView<VulkanResourceView>());
    }

    uint3 VulkanBindSet::GetBoundTextureSize(uint32_t index) const
    {
        if (index < m_set.GetCount())
        {
            auto& extent = m_set[index].image.extent;
            return { extent.width, extent.height, extent.depth };
        }

        return PK_UINT3_ZERO;
    }

    int32_t VulkanBindSet::Add(const VulkanResourceView& handle)
    {
        if (handle.hash == 0ull || m_set.GetCount() >= m_set.GetCapacity())
        {
            return -1;
        }

        const auto& previous = m_set[m_set.GetCount()];
        const auto hasChanged = previous != handle;

        uint32_t index = 0u;
        if (m_set.Add(handle, &index))
        {
            m_isDirty |= hasChanged;
        }

        return index;
    }
}
