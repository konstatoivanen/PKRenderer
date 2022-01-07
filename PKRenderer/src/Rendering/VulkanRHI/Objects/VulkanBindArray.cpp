#include "PrecompiledHeader.h"
#include "VulkanBindArray.h"
#include "Rendering/VulkanRHI/Objects/VulkanTexture.h"
#include "Rendering/VulkanRHI/Objects/VulkanBuffer.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    VulkanBindArray::VulkanBindArray(size_t capacity) : m_capacity(capacity)
    {
        m_handles = reinterpret_cast<const VulkanBindHandle**>(malloc(sizeof(const VulkanBindHandle*) * capacity));
        memset(m_handles, 0, sizeof(const VulkanBindHandle*) * capacity);
    }

    VulkanBindArray::~VulkanBindArray()
    {
        free(m_handles);
    }

    const VulkanBindHandle* const* VulkanBindArray::GetHandles(uint32_t* version, uint32_t* count) const
    {
        if (m_count != m_previousCount)
        {
            m_previousCount = m_count;
            m_isDirty = true;
        }

        if (m_isDirty)
        {
            ++m_version;
            m_isDirty = false;
        }

        *count = (uint32_t)m_count;
        *version = m_version;
        return m_handles;
    }

    int32_t VulkanBindArray::Add(Texture* value, void* bindInfo)
    {
        if (bindInfo)
        {
            return Add(value->GetNative<VulkanTexture>()->GetBindHandle(*reinterpret_cast<TextureViewRange*>(bindInfo), true));
        }

        return Add(value);
    }

    int32_t VulkanBindArray::Add(Texture* value)
    {
        return Add(value->GetNative<VulkanTexture>()->GetBindHandle());
    }

    int32_t VulkanBindArray::Add(const Texture* value)
    {
        return Add(value->GetNative<VulkanTexture>()->GetBindHandle());
    }

    int32_t VulkanBindArray::Add(Buffer* value, void* bindInfo)
    {
        if (bindInfo)
        {
            return Add(value->GetNative<VulkanBuffer>()->GetBindHandle(*reinterpret_cast<IndexRange*>(bindInfo)));
        }

        return Add(value);
    }

    int32_t VulkanBindArray::Add(Buffer* value)
    {
        return Add(value->GetNative<VulkanBuffer>()->GetBindHandle());
    }

    int32_t VulkanBindArray::Add(const Buffer* value)
    {
        return Add(value->GetNative<VulkanBuffer>()->GetBindHandle());
    }

    int32_t VulkanBindArray::Add(const VulkanBindHandle* handle)
    {
        if (handle == nullptr || m_count >= m_capacity)
        {
            return -1;
        }

        auto& target = m_handles[m_count];

        if (target != handle || target->version != handle->version)
        {
            m_isDirty = true;
        }

        m_handles[m_count++] = handle;
        return (int32_t)(m_count) - 1;
    }
}