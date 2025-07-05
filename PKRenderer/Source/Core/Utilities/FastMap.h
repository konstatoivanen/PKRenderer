#pragma once
#include "Hash.h"
#include "NoCopy.h"
#include "BufferView.h"
#include "MemoryBlock.h"
#include "BufferIterator.h"
#include "ContainerHelpers.h"

namespace PK
{
    struct IFastSetNode
    {
        int32_t previous;
        int32_t next;
        IFastSetNode() : previous(-1), next(-1) {}
        IFastSetNode(int32_t previous) : previous(previous), next(-1) {}
    };

    template<typename TKey>
    struct IFastMapNode
    {
        TKey key = {};
        size_t hashcode = 0ull;
        int32_t previous;
        int32_t next;
        IFastMapNode() : previous(-1), next(-1) {}
        IFastMapNode(const TKey& key, uint64_t hash) : key(key), hashcode(hash), previous(-1), next(-1) {}
    };


    template<typename TValue, typename TNode, bool is_fixed, size_t capacity, size_t bucket_count_factor>
    struct IFastCollectionBase;

    // Dynamic allocation base
    template<typename TValue, typename TNode>
    struct IFastCollectionBase<TValue, TNode, false, 0, 1ull> : public NoCopy
    {
    protected:
        void* m_buffer = nullptr;

        union
        {
            int32_t* m_buckets = nullptr;
            int32_t m_bucketsInline;
        };

        TValue* m_values = nullptr;
        TNode* m_nodes = nullptr;
        uint32_t m_bucketCountFactor = 1u;
        uint32_t m_bucketCount = 1u;
        uint32_t m_collisions = 0u;
        uint32_t m_capacity = 0u;
        uint32_t m_count = 0u;
        
        ~IFastCollectionBase() 
        {
            if (m_buffer)
            {
                ContainerHelpers::ClearArray(m_values, m_count);
                ContainerHelpers::ClearArray(m_nodes, m_count);
                free(m_buffer);
            }
        }

        uint32_t GetBucketCount() const { return m_bucketCount; }
        const int32_t* GetBuckets() const { return m_buffer ? m_buckets : &m_bucketsInline; }
        int32_t* GetBuckets() { return m_buffer ? m_buckets : &m_bucketsInline; }

    public:

        bool Reserve(uint32_t count)
        {
            if (m_capacity < count)
            {
                m_capacity = m_capacity == 0 ? count : Hash::ExpandPrime(count);
                m_bucketCount = m_bucketCountFactor * m_capacity;
                
                size_t size = 0ull;
                const auto offsetBuckets = ContainerHelpers::AlignSize<int32_t>(&size);
                size += sizeof(int32_t) * m_bucketCount;
                const auto offsetNode = ContainerHelpers::AlignSize<TNode>(&size);
                size += sizeof(TNode) * m_capacity;
                const auto offsetValue = ContainerHelpers::AlignSize<TValue>(&size);
                size += sizeof(TValue) * m_capacity;
                
                auto newBuffer = calloc(size, 1u);
                auto newBuckets = ContainerHelpers::CastOffsetPtr<int32_t>(newBuffer, offsetBuckets);
                auto newNodes = ContainerHelpers::CastOffsetPtr<TNode>(newBuffer, offsetNode);
                auto newValues = ContainerHelpers::CastOffsetPtr<TValue>(newBuffer, + offsetValue);

                if (m_buffer)
                {
                    ContainerHelpers::MoveArray(newValues, m_values, m_count);
                    ContainerHelpers::MoveArray(newNodes, m_nodes, m_count);
                    free(m_buffer);
                }

                m_buffer = newBuffer;
                m_values = newValues;
                m_nodes = newNodes;
                m_buckets = newBuckets;
                return true;
            }

            return false;
        }

        bool Reserve(uint32_t count, uint32_t bucketCountFactor)
        {
            m_bucketCountFactor = bucketCountFactor > 0 ? bucketCountFactor : 1ull;
            return Reserve(count);
        }
    };

    // Static allocation base
    template<typename TValue, typename TNode, size_t capacity, size_t bucket_count_factor>
    struct IFastCollectionBase<TValue, TNode, true, capacity, bucket_count_factor> : public NoCopy
    {
    protected:
        static constexpr uint32_t m_capacity = capacity;
        int32_t m_buckets[capacity * bucket_count_factor]{};
        TNode m_nodes[capacity]{};
        TValue m_values[capacity]{};
        uint32_t m_collisions = 0u;
        uint32_t m_count = 0u;
        uint32_t GetBucketCount() const { return m_capacity * bucket_count_factor; }
        const int32_t* GetBuckets() const { return m_buckets; }
        int32_t* GetBuckets() { return m_buckets; }
    public: 
        bool Reserve(uint32_t newCount)
        {
            if (m_capacity < newCount)
            {
                throw std::exception("Fixed set capacity exceeded!");
            }

            return false;
        }

        bool Reserve(uint32_t count, uint32_t bucketCountFactor)
        {
            return Reserve(count);
        }
    };


    template<typename TValue, typename THash, bool is_fixed, size_t capacity, size_t bucket_count_factor>
    struct IFastSet : public IFastCollectionBase<TValue, IFastSetNode, is_fixed, capacity, bucket_count_factor>
    {
        using TBase = IFastCollectionBase<TValue, IFastSetNode, is_fixed, capacity, bucket_count_factor>;
        using TBase::m_values;
        using TBase::m_nodes;
        using TBase::m_count;
        using TBase::m_collisions;
        using TBase::m_capacity;
        using Node = IFastSetNode;
        inline static THash Hash;

        IFastSet(uint32_t size, uint32_t bucketCountFactor) { TBase::Reserve(size, bucketCountFactor); }
        IFastSet() : IFastSet(0u, 1u) {}

        constexpr uint32_t GetCount() const { return m_count; }
        constexpr uint32_t GetCapacity() const { return m_capacity; }
        constexpr const TValue* GetValues() const { return m_values; }
        TValue* GetValues() { return m_values; }
        ConstBufferIterator<TValue> begin() const { return ConstBufferIterator<TValue>(m_values, 0ull); }
        ConstBufferIterator<TValue> end() const { return ConstBufferIterator<TValue>(m_values + m_count, m_count); }
        const TValue& operator[](uint32_t index) const { return m_values[index]; }
        TValue& operator[](uint32_t index) { return m_values[index]; }

        int32_t GetHashIndex(size_t hash) const
        {
            if (m_count > 0)
            {
                const auto bucketIndex = GetBucketIndex(hash);
                auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

                while (valueIndex != -1)
                {
                    if (Hash(m_values[valueIndex]) == hash)
                    {
                        return valueIndex;
                    }

                    valueIndex = m_nodes[valueIndex].previous;
                }
            }

            return -1;
        }

        int32_t GetIndex(const TValue& value) const
        {
            if (m_count > 0)
            {
                const auto hash = Hash(value);
                const auto bucketIndex = GetBucketIndex(hash);
                auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

                while (valueIndex != -1)
                {
                    if (m_values[valueIndex] == value)
                    {
                        return valueIndex;
                    }

                    valueIndex = m_nodes[valueIndex].previous;
                }
            }

            return -1;
        }

        bool Contains(const TValue& value) const { return GetIndex(value) != -1; }

        bool Add(const TValue& value, uint32_t* outIndex)
        {
            const auto hash = Hash(value);
            const auto bucketIndex = GetBucketIndex(hash);
            const auto valueIndex = GetValueIndexFromBuckets(bucketIndex);
            auto movingValueIndex = valueIndex;

            while (movingValueIndex != -1)
            {
                if (m_values[movingValueIndex] == value)
                {
                    *outIndex = movingValueIndex;
                    return false;
                }

                movingValueIndex = m_nodes[movingValueIndex].previous;
            }
                
            const auto resized = TBase::Reserve(m_count + 1u);
            const auto index = m_count++;
            m_values[index] = value;

            if (!resized)
            {
                m_nodes[index] = Node(valueIndex);

                if (valueIndex != -1)
                {
                    m_collisions++;
                    m_nodes[valueIndex].next = index;
                }

                SetValueIndexInBuckets(bucketIndex, index);
            }
            else // Dead code elimination should remove this for fixed versions.
            {
                m_collisions = 0u;

                for (auto newValueIndex = 0u; newValueIndex < m_count; newValueIndex++)
                {
                    const auto existingBucketIndex = GetBucketIndex(Hash(m_values[newValueIndex]));
                    const auto existingValueIndex = GetValueIndexFromBuckets(existingBucketIndex);
                    SetValueIndexInBuckets(existingBucketIndex, newValueIndex);

                    if (existingValueIndex != -1)
                    {
                        ++m_collisions;
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

            *outIndex = index;
            return true;
        }

        uint32_t Add(const TValue& value)
        {
            uint32_t outIndex = 0u;
            Add(value, &outIndex);
            return outIndex;
        }

        bool RemoveAt(uint32_t index)
        {
            if (index >= m_count)
            {
                return false;
            }

            const auto hash = Hash(m_values[index]);
            const auto bucketIndex = GetBucketIndex(hash);

            if (GetValueIndexFromBuckets(bucketIndex) == (int32_t)index)
            {
                SetValueIndexInBuckets(bucketIndex, m_nodes[index].previous);
            }

            const auto updateNext = m_nodes[index].next;
            const auto updatePrevious = m_nodes[index].previous;

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
                const auto movingBucketIndex = GetBucketIndex(Hash(m_values[m_count]));

                if (GetValueIndexFromBuckets(movingBucketIndex) == (int32_t)m_count)
                {
                    SetValueIndexInBuckets(movingBucketIndex, index);
                }

                const auto next = m_nodes[m_count].next;
                const auto previous = m_nodes[m_count].previous;

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
                ContainerHelpers::ClearArray(m_values, m_count);
                ContainerHelpers::ClearArray(m_nodes, m_count);
                ClearBuckets();
                m_collisions = 0u;
                m_count = 0u;
            }
        }

        void ClearFast()
        {
            if (m_count > 0)
            {
                ClearBuckets();
                m_collisions = 0u;
                m_count = 0u;
            }
        }

    private:
        uint32_t GetBucketIndex(uint64_t hash) const { return (uint32_t)(hash % TBase::GetBucketCount()); }
        void SetValueIndexInBuckets(uint32_t i, int32_t value) { TBase::GetBuckets()[i] = value + 1; }
        int32_t GetValueIndexFromBuckets(uint32_t i) const { return TBase::GetBuckets()[i] - 1; }
        void ClearBuckets() { memset(TBase::GetBuckets(), 0, sizeof(int32_t) * TBase::GetBucketCount()); }
    };

    template<typename TKey, typename TValue, typename THash, bool is_fixed, size_t capacity, size_t bucket_count_factor>
    struct IFastMap : public IFastCollectionBase<TValue, IFastMapNode<TKey>, is_fixed, capacity, bucket_count_factor>
    {
        using TBase = IFastCollectionBase<TValue, IFastMapNode<TKey>, is_fixed, capacity, bucket_count_factor>;
        using Node = IFastMapNode<TKey>;
        using TBase::m_values;
        using TBase::m_nodes;
        using TBase::m_count;
        using TBase::m_collisions;
        using TBase::m_capacity;
        inline static THash Hash;

        struct KeyValueConst
        {
            const TKey& key;
            const TValue& value;
            KeyValueConst(const TKey& key, const TValue& value) : key(key), value(value) {}
        };

        struct KeyValue
        {
            TKey& key;
            TValue& value;
            KeyValue(TKey& key, TValue& value) : key(key), value(value){}
        };

        IFastMap(uint32_t size, uint32_t bucketCountFactor) { TBase::Reserve(size, bucketCountFactor); }
        IFastMap() : IFastMap(0u, 1u) {}

        constexpr uint32_t GetCount() const { return m_count; }
        constexpr size_t GetCapacity() const { return m_capacity; }
        constexpr const TValue* GetValues() const { return m_values; }
        TValue* GetValues() { return m_values; }
        ConstBufferIterator<TValue> begin() const { return ConstBufferIterator<TValue>(m_values, 0ull); }
        ConstBufferIterator<TValue> end() const { return ConstBufferIterator<TValue>(m_values + m_count, m_count); }
        const KeyValueConst operator[](uint32_t index) const { return { m_nodes[index].key, m_values[index] }; }
        KeyValue operator[](uint32_t index) { return { m_nodes[index].key, m_values[index] }; }

        const TValue* GetValuePtr(const TKey& key) const { auto index = GetIndex(key); return index != -1 ? &m_values[index] : nullptr; }
        TValue* GetValuePtr(const TKey& key) { auto index = GetIndex(key); return index != -1 ? &m_values[index] : nullptr; }
        void SetValue(const TKey& key, const TValue& value) { auto index = GetIndex(key); if (index != -1) m_values[index] = value; }

        int32_t GetIndex(const TKey& key) const
        {
            if (m_count > 0)
            {
                const auto hash = Hash(key);
                const auto bucketIndex = GetBucketIndex(hash);
                auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

                while (valueIndex != -1)
                {
                    if (m_nodes[valueIndex].hashcode == hash && m_nodes[valueIndex].key == key)
                    {
                        return valueIndex;
                    }

                    valueIndex = m_nodes[valueIndex].previous;
                }
            }

            return -1;
        }

        bool Contains(const TKey& key) const { return GetIndex(key) != -1; }

        bool AddKey(const TKey& key, uint32_t* outIndex)
        {
            const auto hash = Hash(key);
            const auto bucketIndex = GetBucketIndex(hash);
            const auto valueIndex = GetValueIndexFromBuckets(bucketIndex);
            auto movingValueIndex = valueIndex;

            while (movingValueIndex != -1)
            {
                if (m_nodes[movingValueIndex].hashcode == hash && m_nodes[movingValueIndex].key == key)
                {
                    *outIndex = movingValueIndex;
                    return false;
                }

                movingValueIndex = m_nodes[movingValueIndex].previous;
            }

            const auto resized = TBase::Reserve(m_count + 1u);
            const auto index = m_count++;
            m_nodes[index] = Node(key, hash);

            if (!resized)
            {
                m_nodes[index].previous = valueIndex;

                if (valueIndex != -1)
                {
                    m_collisions++;
                    m_nodes[valueIndex].next = index;
                }

                SetValueIndexInBuckets(bucketIndex, index);
            }
            else // Dead code elimination should remove this for fixed versions.
            {
                m_collisions = 0;

                for (auto newValueIndex = 0u; newValueIndex < m_count; newValueIndex++)
                {
                    const auto existingBucketIndex = GetBucketIndex(m_nodes[newValueIndex].hashcode);
                    const auto existingValueIndex = GetValueIndexFromBuckets(existingBucketIndex);
                    SetValueIndexInBuckets(existingBucketIndex, newValueIndex);

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

            *outIndex = index;
            return true;
        }

        uint32_t AddKey(const TKey& key)
        {
            uint32_t newIndex = 0u;
            AddKey(key, &newIndex);
            return newIndex;
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

            const auto bucketIndex = GetBucketIndex(m_nodes[index].hashcode);

            if (GetValueIndexFromBuckets(bucketIndex) == (int32_t)index)
            {
                SetValueIndexInBuckets(bucketIndex, m_nodes[index].previous);
            }

            const auto updateNext = m_nodes[index].next;
            const auto updatePrevious = m_nodes[index].previous;

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
                const auto movingBucketIndex = GetBucketIndex(m_nodes[m_count].hashcode);

                if (GetValueIndexFromBuckets(movingBucketIndex) == (int32_t)m_count)
                {
                    SetValueIndexInBuckets(movingBucketIndex, index);
                }

                const auto next = m_nodes[m_count].next;
                const auto previous = m_nodes[m_count].previous;

                if (next != -1)
                {
                    m_nodes[next].previous = index;
                }

                if (previous != -1)
                {
                    m_nodes[previous].next = index;
                }

                m_nodes[index] = m_nodes[m_count];
                m_values[index] = std::move(m_values[m_count]);
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

        void Clear()
        {
            if (m_count > 0)
            {
                ContainerHelpers::ClearArray(m_values, m_count);
                ContainerHelpers::ClearArray(m_nodes, m_count);
                ClearBuckets();
                m_collisions = 0u;
                m_count = 0u;
            }
        }

        void ClearFast()
        {
            if (m_count > 0)
            {
                ClearBuckets();
                m_collisions = 0u;
                m_count = 0u;
            }
        }

        private:
            uint32_t GetBucketIndex(uint64_t hash) const { return (uint32_t)(hash % TBase::GetBucketCount()); }
            void SetValueIndexInBuckets(uint32_t i, int32_t value) { TBase::GetBuckets()[i] = value + 1; }
            int32_t GetValueIndexFromBuckets(uint32_t i) const { return TBase::GetBuckets()[i] - 1; }
            void ClearBuckets() { memset(TBase::GetBuckets(), 0, sizeof(int32_t) * TBase::GetBucketCount()); }
    };


    template<typename TValue, typename THash = std::hash<TValue>>
    using FastSet = IFastSet<TValue, THash, false, 0, 1ull>;

    template<typename TValue>
    using PointerSet = IFastSet<TValue*, Hash::TPointerHash<TValue>, false, 0, 1ull>;

    template<typename TKey, typename TValue, typename THash = std::hash<TKey>>
    using FastMap = IFastMap<TKey, TValue, THash, false, 0, 1ull>;

    template<typename TKey, typename TValue, typename THash = std::hash<TKey>>
    using PointerMap = IFastMap<TKey, TValue*, THash, false, 0, 1ull>;


    template<typename TValue, size_t capacity, typename THash = std::hash<TValue>, size_t bucket_count_factor = 1ull>
    using FixedSet = IFastSet<TValue, THash, true, capacity, bucket_count_factor>;

    template<typename TValue, size_t capacity, size_t bucket_count_factor = 1ull>
    using FixedPointerSet = IFastSet<TValue*, Hash::TPointerHash<TValue>, true, capacity, bucket_count_factor>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = std::hash<TKey>, size_t bucket_count_factor = 1ull>
    using FixedMap = IFastMap<TKey, TValue, THash, true, capacity, bucket_count_factor>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = std::hash<TKey>, size_t bucket_count_factor = 1ull>
    using FixedPointerMap = IFastMap<TKey, TValue*, THash, true, capacity, bucket_count_factor>;
}
