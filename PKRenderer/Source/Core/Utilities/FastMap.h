#pragma once
#include "Hash.h"
#include "NoCopy.h"
#include "BufferView.h"
#include "MemoryBlock.h"
#include "BufferIterator.h"

namespace PK
{
    template<typename TKey, typename TValue, typename THash = std::hash<TKey>>
    struct FastMap : NoCopy
    {
        struct Node
        {
            TKey key = {};
            size_t hashcode = 0ull;
            int32_t previous;
            int32_t next;
            Node() : previous(-1), next(-1) {}
            Node(const TKey& key, uint64_t hash, int32_t previousNode) : key(key), hashcode(hash), previous(previousNode), next(-1) {}
            Node(const TKey& key, uint64_t hash) : key(key), hashcode(hash), previous(-1), next(-1) {}
        };

        struct KeyValues
        {
            const Node* nodes;
            TValue* values;
            size_t count;
        };

    private:
        inline static THash Hash;
        //@TODO use one block for values & nodes
        MemoryBlock<TValue> m_values;
        MemoryBlock<Node> m_nodes;
        MemoryBlock<int32_t> m_buckets;
        uint32_t m_collisions;
        uint32_t m_count;

        uint32_t GetBucketIndex(uint64_t hash) const { return (uint32_t)(hash % m_buckets.GetCount()); }
        void SetValueIndexInBuckets(uint32_t i, int32_t value) { m_buckets[i] = value + 1; }
        int32_t GetValueIndexFromBuckets(uint32_t i) const { return m_buckets[i] - 1; }
        void ReserveMemory(uint32_t newSize)
        {
            if (m_values.GetCount() < newSize)
            {
                auto size = Hash::ExpandPrime(newSize);
                m_values.Validate(size);
                m_nodes.Validate(size);
            }

            if (m_buckets.GetCount() == 0 && newSize > 0)
            {
                m_buckets.Validate(Hash::GetPrime(newSize));
            }
        }

    public:
        FastMap(uint32_t size) : m_collisions(0u), m_count(0u) { ReserveMemory(size); }
        FastMap() : FastMap(0u) {}

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

        int32_t GetIndex(const TKey& key) const
        {
            if (m_count == 0)
            {
                return -1;
            }

            size_t hash = Hash(key);
            auto bucketIndex = GetBucketIndex(hash);
            auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

            while (valueIndex != -1)
            {
                if (m_nodes[valueIndex].hashcode == hash && m_nodes[valueIndex].key == key)
                {
                    return valueIndex;
                }

                valueIndex = m_nodes[valueIndex].previous;
            }

            return -1;
        }

        bool AddKey(const TKey& key, uint32_t* outIndex)
        {
            if (m_count == 0)
            {
                ReserveMemory(1u);
            }

            auto hash = Hash(key);
            auto bucketIndex = GetBucketIndex(hash);
            auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

            if (valueIndex == -1)
            {
                ReserveMemory(m_count + 1u);
                m_nodes[m_count] = Node(key, hash);
            }
            else 
            {
                auto currentValueIndex = valueIndex;

                do
                {
                    if (m_nodes[currentValueIndex].hashcode == hash && 
                        m_nodes[currentValueIndex].key == key)
                    {
                        *outIndex = currentValueIndex;
                        return false;
                    }

                    currentValueIndex = m_nodes[currentValueIndex].previous;
                } 
                while (currentValueIndex != -1);

                m_collisions++;
                ReserveMemory(m_count + 1u);
                m_nodes[m_count] = Node(key, hash, valueIndex);
                m_nodes[valueIndex].next = m_count;
            }

            SetValueIndexInBuckets(bucketIndex, m_count++);

            if (m_collisions > m_buckets.GetCount())
            {
                m_buckets.Validate(Hash::ExpandPrime(m_collisions));
                m_buckets.Clear();
                m_collisions = 0;
                
                for (auto newValueIndex = 0u; newValueIndex < m_count; newValueIndex++)
                {
                    bucketIndex = GetBucketIndex(m_nodes[newValueIndex].hashcode);
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

            *outIndex = m_count - 1u;
            return true;
        }

        bool AddValue(const TKey& key, const TValue& value)
        {
            auto index = 0u;
            auto appended = AddKey(key, &index);
            m_values[index] = value;
            return appended;
        }

        bool RemoveAt(uint32_t index)
        {
            if (index >= m_count)
            {
                return false;
            }

            auto bucketIndex = GetBucketIndex(m_nodes[index].hashcode);

            if (GetValueIndexFromBuckets(bucketIndex) == (int32_t)index)
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
                auto movingBucketIndex = GetBucketIndex(m_nodes[m_count].hashcode);

                if (GetValueIndexFromBuckets(movingBucketIndex) == (int32_t)m_count)
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

        bool Remove(const TKey& key)
        {
            auto index = GetIndex(key);

            if (index != -1)
            {
                return RemoveAt(index);
            }

            return index != -1;
        }

        constexpr uint32_t GetCount() const { return m_count; }
        constexpr size_t GetCapacity() const { return m_values.GetCount(); }
        bool Contains(const TKey& key) const { return GetIndex(key) != -1; }
        
        BufferView<TValue> GetValues() { return { m_values.GetData(), (size_t)m_count }; }
        ConstBufferView<TValue> GetValues() const { return { m_values.GetData(), (size_t)m_count }; }
        KeyValues GetKeyValues() { return { m_nodes.GetData(), m_values.GetData(), (size_t)m_count }; }
        
        const TValue* GetValueRef(const TKey& key) const { auto index = GetIndex(key); return index != -1 ? &m_values[index] : nullptr; }
        const TValue& GetValueAt(uint32_t index) const { return m_values[index]; }
        const TKey& GetKeyAt(uint32_t index) const { return m_nodes[index].key; }

        TValue* GetValueRef(const TKey& key) { auto index = GetIndex(key); return index != -1 ? &m_values[index] : nullptr; }
        TValue& GetValueAt(uint32_t index) { return m_values[index]; }
        
        void SetValue(const TKey& key, const TValue& value) { auto index = GetIndex(key); if (index != -1) m_values[index] = value; }
        void SetValueAt(uint32_t index, const TValue& value) { m_values[index] = value; }
    };

    template<typename TKey, typename TValue, typename THash = std::hash<TKey>>
    struct PointerMap : public FastMap<TKey, TValue*, THash>
    {
        PointerMap(uint32_t size) : FastMap<TKey, TValue*, THash>(size) {}
        PointerMap() : FastMap<TKey, TValue*, THash>() {}
    };
}
