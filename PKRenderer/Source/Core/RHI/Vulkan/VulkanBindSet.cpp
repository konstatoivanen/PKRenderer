#include "PrecompiledHeader.h"
#include "Core/RHI/Vulkan/VulkanTexture.h"
#include "Core/RHI/Vulkan/VulkanBuffer.h"
#include "VulkanBindSet.h"

namespace PK
{
    VulkanBindSet::VulkanBindSet(size_t capacity) : m_set(capacity, 1ull)
    {
    }

    const VulkanBindHandle* const* VulkanBindSet::GetHandles(uint32_t* version, uint32_t* count) const
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
            return Add(static_cast<VulkanTexture*>(value)->GetBindHandle(*reinterpret_cast<TextureViewRange*>(bindInfo), TextureBindMode::SampledTexture));
        }

        return Add(value);
    }

    int32_t VulkanBindSet::Add(RHITexture* value)
    {
        return Add(static_cast<VulkanTexture*>(value)->GetBindHandle());
    }

    int32_t VulkanBindSet::Add(RHIBuffer* value, void* bindInfo)
    {
        if (bindInfo)
        {
            return Add(static_cast<VulkanBuffer*>(value)->GetBindHandle(*reinterpret_cast<BufferIndexRange*>(bindInfo)));
        }

        return Add(value);
    }

    int32_t VulkanBindSet::Add(RHIBuffer* value)
    {
        return Add(static_cast<VulkanBuffer*>(value)->GetBindHandle());
    }

    uint3 VulkanBindSet::GetBoundTextureSize(uint32_t index) const
    {
        if (index < m_set.GetCount())
        {
            auto& extent = m_set[index]->image.extent;
            return { extent.width, extent.height, extent.depth };
        }

        return PK_UINT3_ZERO;
    }

    BufferIndexRange VulkanBindSet::GetBoundBufferRange(uint32_t index) const
    {
        if (index < m_set.GetCount())
        {
            auto& buffer = m_set[index]->buffer;
            return { buffer.offset, buffer.range };
        }

        return { 0u, 0u };
    }

    int32_t VulkanBindSet::Add(const VulkanBindHandle* handle)
    {
        if (handle == nullptr || m_set.GetCount() >= m_set.GetCapacity())
        {
            return -1;
        }

        const auto& previous = m_set[m_set.GetCount()];
        const auto hasChanged = previous != handle || previous->Version() != handle->Version();

        uint32_t index = 0u;
        if (m_set.Add(handle, &index))
        {
            m_isDirty |= hasChanged;
        }

        return index;
    }
}
