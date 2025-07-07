#include "PrecompiledHeader.h"
#include "PropertyBlock.h"

namespace PK
{
    PropertyBlock::PropertyBlock(uint64_t capacityBytes, uint64_t capacityProperties) : m_properties(capacityProperties, 4ull)
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
        for (auto i = 0u; i < from.m_properties.GetCount(); ++i)
        {
            const auto& prop = from.m_properties[i];
            auto index = m_properties.GetIndex(prop);

            if (index != -1)
            {
                WriteValueInternal(reinterpret_cast<char*>(from.m_buffer) + prop.offset, (uint32_t)index, prop.size);
            }
        }
    }

    void PropertyBlock::Clear()
    {
        m_head = 0;
        m_properties.Clear();
        memset(m_buffer, 0, m_capacity);
    }

    void PropertyBlock::ClearAndReserve(uint64_t capacityBytes, uint64_t capacityProperties)
    {
        Clear();
        ValidateBufferSize(capacityBytes);
        m_properties.Reserve(capacityProperties);
    }


    void* PropertyBlock::GetValueInternal(uint64_t key, size_t* size)  const
    {
        auto prop = m_properties.GetValuePtr({ key, 0u, 0u });

        if (prop != nullptr)
        {
            if (size != nullptr)
            {
                *size = (uint64_t)prop->size;
            }

            if ((prop->offset + (uint64_t)prop->size) > m_capacity)
            {
                throw std::invalid_argument("Out of bounds array index!");
            }

            return reinterpret_cast<char*>(m_buffer) + prop->offset;
        }

        return nullptr;
    }

    uint32_t PropertyBlock::AddKeyInternal(uint64_t key, size_t size)
    {
        if (m_fixedLayout)
        {
            return (uint32_t)m_properties.GetIndex({ key, 0u, 0u });
        }

        if (size > 0)
        {
            uint32_t index = 0u;
            if (m_properties.Add({ key, 0u, 0u }, &index))
            {
                const auto wsize = (uint32_t)size;
                ValidateBufferSize(m_head + wsize);
                m_properties[index].offset = (uint32_t)m_head;
                m_properties[index].size = wsize;
                m_head += wsize;
            }

            return index;
        }

        return ~0u;
    }

    bool PropertyBlock::WriteValueInternal(const void* src, uint32_t index, size_t writeSize)
    {
        if (index >= m_properties.GetCount())
        {
            return false;
        }

        const auto& prop = m_properties[index];

        if (prop.size < writeSize)
        {
            return false;
        }

        auto dst = reinterpret_cast<char*>(m_buffer) + prop.offset;
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
