#include "PrecompiledHeader.h"
#include "PropertyBlock.h"

namespace PK::Utilities
{
    PropertyBlock::PropertyBlock(uint64_t capacityBytes, uint64_t capacityProperties) : m_propertyInfos(capacityProperties)
    {
        ValidateBufferSize(capacityBytes);
    }

    PropertyBlock::~PropertyBlock()
    {
        if (m_buffer != nullptr)
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
}