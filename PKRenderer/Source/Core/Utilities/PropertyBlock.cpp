#include "PrecompiledHeader.h"
#include "Core/Utilities/Hash.h"
#include "Core/Utilities/ContainerHelpers.h"
#include "PropertyBlock.h"

namespace PK
{
    PropertyBlock::PropertyBlock(uint64_t byteCapacity, uint64_t propertyCapacity)
    {
        ReserveMemory(byteCapacity, propertyCapacity);
    }

    PropertyBlock::~PropertyBlock()
    {
        if (m_buffer != nullptr)
        {
            free(m_buffer);
        }
    }


    void PropertyBlock::Copy(PropertyBlock& from)
    {
        for (auto i = 0u; i < from.m_propertyCount; ++i)
        {
            const auto& prop = from.m_properties[i];
            const auto bucketIndex = GetBucketIndex(prop.key);
            auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

            while (valueIndex != -1)
            {
                if (m_properties[valueIndex].key == prop.key)
                {
                    const auto src = reinterpret_cast<char*>(from.m_buffer) + prop.offset;
                    WriteValue(src, (uint32_t)valueIndex, prop.size);
                    break;
                }

                valueIndex = m_properties[valueIndex].previous;
            }
        }
    }

    void PropertyBlock::Clear()
    {
        memset(GetBuckets(), 0, sizeof(uint16_t) * m_bucketCount);
        memset(m_properties, 0, sizeof(Property) * m_propertyCount);
        memset(m_buffer, 0, sizeof(char) * m_bufferHead);
        m_bufferHead = 0ull;
        m_propertyCount = 0;
    }

    void PropertyBlock::ClearAndReserve(uint64_t byteCapacity, uint64_t propertyCapacity)
    {
        Clear();
        ReserveMemory(byteCapacity, propertyCapacity);
    }


    const void* PropertyBlock::GetValuePtr(uint64_t key, size_t* size) const
    {
        const auto bucketIndex = GetBucketIndex(key);
        auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

        while (valueIndex != -1)
        {
            if (m_properties[valueIndex].key == key)
            {
                const auto& prop = m_properties[valueIndex];

                if (size != nullptr)
                {
                    *size = (uint64_t)prop.size;
                }

                return reinterpret_cast<const char*>(m_buffer) + prop.offset;
            }

            valueIndex = m_properties[valueIndex].previous;
        }

        return nullptr;
    }

    uint32_t PropertyBlock::AddKey(uint64_t key, size_t size)
    {
        const auto bucketIndex = GetBucketIndex(key);
        const auto valueIndex = GetValueIndexFromBuckets(bucketIndex);
        auto movingValueIndex = valueIndex;

        while (movingValueIndex != -1)
        {
            if (m_properties[movingValueIndex].key == key)
            {
                return movingValueIndex;
            }

            movingValueIndex = m_properties[movingValueIndex].previous;
        }

        if (size <= 0ull)
        {
            return ~0u;
        }

        const auto resized = ReserveMemory(m_bufferHead + size, m_propertyCount + 1u);
        const auto index = m_propertyCount++;

        m_properties[index].key = key;
        m_properties[index].offset = (uint32_t)m_bufferHead;
        m_properties[index].size = size;
        m_bufferHead += size;

        if (!resized)
        {
            m_properties[index].previous = valueIndex;
            SetValueIndexInBuckets(bucketIndex, index);
        }
        else
        {
            for (auto newValueIndex = 0u; newValueIndex < m_propertyCount; newValueIndex++)
            {
                const auto existingBucketIndex = GetBucketIndex(m_properties[newValueIndex].key);
                const auto existingValueIndex = GetValueIndexFromBuckets(existingBucketIndex);
                SetValueIndexInBuckets(existingBucketIndex, newValueIndex);
                m_properties[newValueIndex].previous = existingValueIndex;
            }
        }

        return index;
    }

    bool PropertyBlock::WriteValue(const void* src, uint32_t index, size_t writeSize)
    {
        if (index < m_propertyCount && m_properties[index].size >= writeSize)
        {
            auto dst = reinterpret_cast<char*>(m_buffer) + m_properties[index].offset;
            memcpy(dst, src, writeSize);
            return true;
        }

        return true;
    }

    bool PropertyBlock::ReserveMemory(uint64_t byteCapacity, uint32_t propertyCapacity)
    {
        bool resize = false;

        if (m_propertyCapacity < propertyCapacity)
        {
            propertyCapacity = m_propertyCapacity == 0 ? propertyCapacity : Hash::ExpandPrime(propertyCapacity);
            resize = true;
        }

        if (m_bufferSize < byteCapacity)
        {
            byteCapacity = m_bufferSize == 0 ? byteCapacity : Hash::ExpandPrime(byteCapacity);
            resize = true;
        }

        if (resize)
        {
            size_t size = 0ull;
            size += sizeof(char) * byteCapacity;
            const auto offsetBuckets = ContainerHelpers::AlignSize<uint64_t>(&size);
            size += sizeof(uint16_t) * (propertyCapacity * BUCKET_COUNT_FACTOR);
            const auto offsetProperties = ContainerHelpers::AlignSize<Property>(&size);
            size += sizeof(Property) * propertyCapacity;

            auto newBuffer = calloc(size, 1u);
            auto newBuckets = ContainerHelpers::CastOffsetPtr<uint16_t>(newBuffer, offsetBuckets);
            auto newProperties = ContainerHelpers::CastOffsetPtr<Property>(newBuffer, offsetProperties);

            if (m_buffer)
            {
                memcpy(newBuffer, m_buffer, sizeof(char) * m_bufferSize);
                memcpy(newProperties, m_properties, sizeof(Property) * m_propertyCapacity);
                free(m_buffer);
            }

            m_bucketCount = (propertyCapacity * BUCKET_COUNT_FACTOR);
            m_propertyCapacity = propertyCapacity;
            m_bufferSize = byteCapacity;
            m_buffer = newBuffer;
            m_properties = newProperties;
            m_buckets = newBuckets;
        }

        return resize;
    }
}
