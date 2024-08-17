#pragma once
#include "Hash.h"
#include "NoCopy.h"
#include "BufferView.h"
#include "MemoryBlock.h"
#include "BufferIterator.h"

namespace PK
{
    template<typename TValue, typename THash = std::hash<TValue>>
    struct FastSet : NoCopy
    {
        struct Node
        {
            int32_t previous;
            int32_t next;
            Node() : previous(-1), next(-1) {}
            Node(int32_t previous) : previous(previous), next(-1) {}
        };

    private:
        inline static THash Hash;
        // @TODO use a single block for both values and nodes for better cache locality.
        MemoryBlock<TValue> m_values;
        MemoryBlock<Node> m_nodes;
        MemoryBlock<int32_t> m_buckets;
        uint32_t m_collisions;
        uint32_t m_count;

        uint32_t GetBucketIndex(uint64_t hash) const { return (uint32_t)(hash % m_buckets.GetCount()); }
        void SetValueIndexInBuckets(uint32_t i, int32_t value) { m_buckets[i] = value + 1; }
        int32_t GetValueIndexFromBuckets(uint32_t i) const  { return m_buckets[i] - 1; }

        void ReserveMemory(uint32_t newSize)
        {
            if (m_values.GetCount() < newSize)
            {
                m_values.Validate(Hash::ExpandPrime(newSize));
                m_nodes.Validate(Hash::ExpandPrime(newSize));
            }

            if (m_buckets.GetCount() == 0 && newSize > 0)
            {
                m_buckets.Validate(Hash::GetPrime(newSize));
            }
        }

    public:
        FastSet(uint32_t size) : m_collisions(0u), m_count(0u) { ReserveMemory(size); }
        FastSet() : FastSet(0u) {}

        FastSet(const FastSet& other) : FastSet(other.GetCount()) 
        {
            m_values.CopyFrom(other.m_values);
            m_nodes.CopyFrom(other.m_nodes);
            m_buckets.CopyFrom(other.m_buckets);
            m_collisions = other.m_collisions;
            m_count = other.m_count;
        }

        int32_t GetHashIndex(size_t hash) const
        {
            if (m_count == 0)
            {
                return -1;
            }

            auto bucketIndex = GetBucketIndex(hash);
            auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

            while (valueIndex != -1)
            {
                if (Hash(m_values[valueIndex]) == hash)
                {
                    return valueIndex;
                }

                valueIndex = m_nodes[valueIndex].previous;
            }

            return -1;
        }
        
        int32_t GetIndex(const TValue& value) const
        { 
            if (m_count == 0)
            {
                return -1;
            }

            size_t hash = Hash(value);
            auto bucketIndex = GetBucketIndex(hash);
            auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

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

        bool Add(const TValue& value, uint32_t& outIndex)
        {
            if (m_count == 0)
            {
                ReserveMemory(1u);
            }

            size_t hash = Hash(value);
            auto bucketIndex = GetBucketIndex(hash);
            auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

            if (valueIndex == -1)
            {
                ReserveMemory(m_count + 1u);
                m_nodes[m_count] = Node();
            }
            else 
            {
                auto currentValueIndex = valueIndex;

                do
                {
                    if (m_values[currentValueIndex] == value)
                    {
                        outIndex = currentValueIndex;
                        return false;
                    }

                    currentValueIndex = m_nodes[currentValueIndex].previous;
                } 
                while (currentValueIndex != -1);

                m_collisions++;
                ReserveMemory(m_count + 1u);
                m_nodes[m_count] = Node(valueIndex);
                m_nodes[valueIndex].next = m_count;
            }

            SetValueIndexInBuckets(bucketIndex, (int32_t)m_count);
            m_values[m_count++] = value;

            if (m_collisions > m_buckets.GetCount())
            {
                m_buckets.Validate(Hash::ExpandPrime(m_collisions));
                m_buckets.Clear();
                m_collisions = 0;

                for (auto newValueIndex = 0u; newValueIndex < m_count; newValueIndex++)
                {
                    bucketIndex = GetBucketIndex(Hash(m_values[newValueIndex]));
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

            outIndex = m_count - 1;
            return true;
        }

        uint32_t Add(const TValue& value)
        {
            uint32_t newIndex = 0u;
            Add(value, newIndex);
            return newIndex;
        }

        bool RemoveAt(uint32_t index)
        {
            if (index >= m_count)
            {
                return false;
            }

            auto hash = Hash(m_values[index]);
            auto bucketIndex = GetBucketIndex(hash);
            
            if (GetValueIndexFromBuckets(bucketIndex) == index)
            {
                SetValueIndexInBuckets(bucketIndex, m_nodes[index].previous);
            }

            auto updateNext = m_nodes[index].next;
            auto updatePrevious = m_nodes[index].previous;

            if (updateNext != -1)
            {
                m_nodes[updateNext].previous = updatePrevious;
            }

            if (updatePrevious != -1)
            {
                m_collisions--;
                m_nodes[updatePrevious].next = updateNext;
            }

            m_count--; 

            if (index != m_count)
            {   
                auto movingBucketIndex = GetBucketIndex(Hash(m_values[m_count]));
                
                if (GetValueIndexFromBuckets(movingBucketIndex) == m_count)
                {
                    SetValueIndexInBuckets(movingBucketIndex, index);
                }

                auto next = m_nodes[m_count].next;
                auto previous = m_nodes[m_count].previous;

                if (next != -1)
                {
                    m_nodes[next].previous = index;
                }

                if (previous != -1)
                {
                    m_nodes[previous].next = index;
                }

                m_nodes[index] = m_nodes[m_count];
                m_values[index] = m_values[m_count];
            }

            return true;
        }

        bool Remove(const TValue& value)
        {
            auto index = GetIndex(value);

            if (index != -1)
            {
                return RemoveAt(index);
            }

            return index != -1;
        }

        void Clear()
        {
            if (m_count > 0)
            {
                m_count = 0u;
                m_collisions = 0u;
                m_values.Clear();
                m_nodes.Clear();
                m_buckets.Clear();
            }
        }

        void ClearFast()
        {
            if (m_count > 0)
            {
                m_count = 0u;
                m_collisions = 0u;
                m_buckets.Clear();
            }
        }

        void Reserve(uint32_t size)
        {
            m_values.Validate(size);
            m_nodes.Validate(size);
            m_nodes.Validate(size);
            m_buckets.Validate(Hash::GetPrime(size));
        }

        constexpr uint32_t GetCount() const { return m_count; }
        constexpr size_t GetCapacity() const { return m_values.GetCount(); }

        ConstBufferIterator<TValue> begin() const { return ConstBufferIterator<TValue>(m_values.GetData(), 0ull); }
        ConstBufferIterator<TValue> end() const { return ConstBufferIterator<TValue>(m_values.GetData() + m_count, m_count); }
        ConstBufferView<TValue> GetValues() const { return { m_values.GetData(), (size_t)m_count }; }
        BufferView<TValue> GetValues() { return { m_values.GetData(), (size_t)m_count }; }

        const TValue& GetValue(uint32_t index) const { return m_values[index]; }
        const TValue& operator[](uint32_t index) const { return m_values[index]; }

        TValue& GetValue(uint32_t index) { return m_values[index]; }
        TValue& operator[](uint32_t index) { return m_values[index]; }
    };

    template<typename TValue, typename THash = Hash::TPointerHash<TValue>>
    struct PointerSet : public FastSet<TValue*, THash>
    {
        PointerSet(uint32_t size) : FastSet<TValue*, THash>(size) {}
        PointerSet() : FastSet<TValue*, THash>() {}
    };
}