#pragma once
#include "Hash.h"
#include "NoCopy.h"
#include "BufferView.h"
#include "MemoryBlock.h"
#include "BufferIterator.h"

namespace PK::Utilities
{
    template<typename T>
    struct IndexedSet : NoCopy
    {
        using TValue = const T*;

    private:
        struct Node
        {
            int32_t previous;
            int32_t next;
            Node() : previous(-1), next(-1) {}
            Node(int32_t previous) : previous(previous), next(-1) {}
        };

        MemoryBlock<TValue> m_values;
        MemoryBlock<Node> m_nodes;
        MemoryBlock<int32_t> m_buckets;
        uint32_t m_collisions;
        uint32_t m_count;

        uint32_t GetBucketIndex(TValue value) const { return (uint32_t)((size_t)value % m_buckets.GetCount()); }
        void SetValueIndexInBuckets(uint32_t i, int32_t value) { m_buckets[i] = value + 1; }
        int32_t GetValueIndexFromBuckets(uint32_t i) const  { return m_buckets[i] - 1; }

    public:
        IndexedSet(uint32_t size) : 
            m_values(size), 
            m_nodes(size),
            m_buckets(Hash::GetPrime(size)),
            m_collisions(0u), 
            m_count(0u)
        {
        }

        IndexedSet() : IndexedSet(1)
        {}

        void Clear()
        {
            if (m_count == 0)
            {
                return;
            }

            m_count = 0u;
            m_values.Clear();
            m_nodes.Clear();
            m_buckets.Clear();
        }

        int32_t GetIndex(TValue value) const
        { 
            auto valueIndex = GetValueIndexFromBuckets(GetBucketIndex(value));

            while (valueIndex != -1)
            {
                if (m_values[valueIndex] == value)
                {
                    return valueIndex;
                }

                valueIndex = m_nodes[valueIndex].previous;
            }

            return -1;
        }

        bool Add(TValue value, uint32_t* outIndex)
        {
            auto bucketIndex = GetBucketIndex(value);
            auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

            if (valueIndex == -1)
            {
                m_nodes[m_count] = Node();
            }
            else 
            {
                auto currentValueIndex = valueIndex;

                do
                {
                    if (m_values[currentValueIndex] == value)
                    {
                        if (outIndex != nullptr)
                        {
                            *outIndex = currentValueIndex;
                        }

                        return false;
                    }

                    currentValueIndex = m_nodes[currentValueIndex].previous;
                } 
                while (currentValueIndex != -1);

                m_collisions++;
                m_nodes[m_count] = Node(valueIndex);
                m_nodes[valueIndex].next = m_count;
            }

            SetValueIndexInBuckets(bucketIndex, (int32_t)m_count);
            m_values[m_count++] = value;

            if (m_count == m_values.GetCount())
            {
                m_values.Validate(Hash::ExpandPrime(m_count));
                m_nodes.Validate(Hash::ExpandPrime(m_count));
            }

            if (m_collisions > m_buckets.GetCount())
            {
                m_buckets.Validate(Hash::ExpandPrime(m_collisions), true);
                m_buckets.Clear();
                m_collisions = 0;

                for (auto newValueIndex = 0u; newValueIndex < m_count; newValueIndex++)
                {
                    bucketIndex = GetBucketIndex(m_values[newValueIndex]);
                    auto existingValueIndex = GetValueIndexFromBuckets(bucketIndex);
                    SetValueIndexInBuckets(bucketIndex, newValueIndex);

                    if (existingValueIndex != -1)
                    { 
                        m_collisions++;
                        m_nodes[newValueIndex].previous = existingValueIndex;
                        m_nodes[newValueIndex].next = -1;
                        m_nodes[existingValueIndex].next = newValueIndex;
                    }
                    else
                    { 
                        m_nodes[newValueIndex].next = -1;
                        m_nodes[newValueIndex].previous = -1;
                    }
                }
            }

            if (outIndex != nullptr)
            {
                *outIndex = m_count - 1;
            }

            return true;
        }

        uint32_t Add(TValue value)
        {
            uint32_t newIndex = 0u;
            Add(value, &newIndex);
            return newIndex;
        }

        bool Remove(TValue value)
        {
            auto bucketIndex = GetBucketIndex(value);
            auto indexToValueToRemove = GetValueIndexFromBuckets(bucketIndex);
            
            while (indexToValueToRemove != -1)
            {
                if (m_values[indexToValueToRemove] == value)
                {
                    if (GetValueIndexFromBuckets(bucketIndex) == indexToValueToRemove)
                    {
                        if (m_nodes[indexToValueToRemove].next != -1)
                        {
                            throw std::exception("if the bucket points to the cell, next must not be assigned!");
                        }

                        SetValueIndexInBuckets(bucketIndex, m_nodes[indexToValueToRemove].previous);
                    }
                    else if (m_nodes[indexToValueToRemove].next == -1)
                    {
                        throw std::exception("if the bucket points to another cell, next must be assigned!");
                    }

                    auto updateNext = m_nodes[indexToValueToRemove].next;
                    auto updatePrevious = m_nodes[indexToValueToRemove].previous;

                    if (updateNext != -1)
                    {
                        m_nodes[updateNext].previous = updatePrevious;
                    }

                    if (updatePrevious != -1)
                    {
                        m_nodes[updatePrevious].next = updateNext;
                    }

                    break;
                }

                indexToValueToRemove = m_nodes[indexToValueToRemove].previous;
            }

            if (indexToValueToRemove == -1)
            {
                return false;
            }

            m_count--; 

            if (indexToValueToRemove != m_count)
            {   
                auto movingBucketIndex = GetBucketIndex(m_values[m_count]);
                
                if (GetValueIndexFromBuckets(movingBucketIndex) == m_count)
                {
                    SetValueIndexInBuckets(movingBucketIndex, indexToValueToRemove);
                }

                auto next = m_nodes[m_count].next;
                auto previous = m_nodes[m_count].previous;

                if (next != -1)
                {
                    m_nodes[next].previous = indexToValueToRemove;
                }

                if (previous != -1)
                {
                    m_nodes[previous].next = indexToValueToRemove;
                }

                m_nodes[indexToValueToRemove] = m_nodes[m_count];
                m_values[indexToValueToRemove] = m_values[m_count];
            }

            return true;
        }

        ConstBufferIterator<TValue> begin() const { return ConstBufferIterator<TValue>(m_values.GetData(), 0ull); }
        ConstBufferIterator<TValue> end() const { return ConstBufferIterator<TValue>(m_values.GetData() + m_count, m_count); }
        ConstBufferView<TValue> GetValues() const { return { m_values.GetData(), (size_t)m_count }; }

        constexpr uint32_t GetCount() const { return m_count; }
        constexpr size_t GetCapacity() const { return m_values.GetCount(); }

        TValue GetValue(uint32_t index) const { return m_values[index]; }
        TValue operator[](uint32_t index) const { return m_values[index]; }
    };
}