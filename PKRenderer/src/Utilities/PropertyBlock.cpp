#include "PrecompiledHeader.h"
#include "PropertyBlock.h"

namespace PK::Utilities
{
    PropertyBlock::PropertyBlock(uint64_t initialCapacity)
    {
        ValidateBufferSize(initialCapacity);
    }

    PropertyBlock::PropertyBlock(void* externalBuffer, uint64_t initialCapacity)
    {
        m_capacity = initialCapacity;
        m_buffer = externalBuffer;
        m_externalBuffer = true;
    }

    PropertyBlock::~PropertyBlock()
    {
        if (m_buffer != nullptr && !m_externalBuffer)
        {
            free(m_buffer);
        }
    }

    void PropertyBlock::CopyFrom(PropertyBlock& from)
    {
        auto keyvalues = from.m_propertyInfos.GetKeyValues();

        for (auto i = 0u; i < keyvalues.count; ++i)
        {
            auto index = m_propertyInfos.GetIndex(keyvalues.nodes[i].key);

            if (index != -1)
            {
                const auto& info = keyvalues.values[i];
                TryWriteValue(reinterpret_cast<char*>(from.m_buffer) + info.offset, (uint32_t)index, info.size);
            }
        }
    }

    void PropertyBlock::Clear()
    {
        m_head = 0;
        m_propertyInfos.Clear();
        memset(m_buffer, 0, m_capacity);
    }

    bool PropertyBlock::TryWriteValue(const void* src, uint32_t index, uint64_t writeSize)
    {
        if (index >= m_propertyInfos.GetCount())
        {
            return false;
        }

        const auto& info = m_propertyInfos.GetValueAt(index);

        if (info.size < writeSize)
        {
            return false;
        }

        auto dst = reinterpret_cast<char*>(m_buffer) + info.offset;
        memcpy(dst, src, writeSize);
        return true;
    }

    void PropertyBlock::ValidateBufferSize(uint64_t size)
    {
        if (size <= m_capacity)
        {
            return;
        }

        if (m_externalBuffer)
        {
            throw std::runtime_error("Cannot resize an external buffer!");
        }

        auto newBuffer = calloc(1, size);

        if (newBuffer == nullptr)
        {
            throw std::runtime_error("Failed to allocate new buffer!");
        }

        if (m_buffer != nullptr)
        {
            memcpy(newBuffer, m_buffer, m_capacity);
            free(m_buffer);
        }

        m_capacity = size;
        m_buffer = newBuffer;
    }

    void PropertyBlock::SetExternalBuffer(void* externalBuffer, uint64_t capacity)
    {
        m_buffer = externalBuffer;
        m_capacity = capacity;
        m_externalBuffer = true;
        Clear();
    }
}