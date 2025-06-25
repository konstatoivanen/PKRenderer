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
        IFastMapNode(const TKey& key, uint64_t hash, int32_t previousNode) : key(key), hashcode(hash), previous(previousNode), next(-1) {}
        IFastMapNode(const TKey& key, uint64_t hash) : key(key), hashcode(hash), previous(-1), next(-1) {}
    };


    template<typename TValue, typename TNode, typename THash, bool is_fixed, size_t capacity>
    struct IFastCollectionBase;

    template<typename TValue, typename TNode, typename THash>
    struct IFastCollectionBase<TValue, TNode, THash, false, 0> : public NoCopy
    {
    protected:
        MemoryBlock<int32_t> m_buckets;
        void* m_buffer = nullptr;
        TValue* m_values = nullptr;
        TNode* m_nodes = nullptr;
        uint32_t m_capacity = 0u;
        uint32_t m_count = 0u;
        uint32_t m_collisions = 0u;
        
        ~IFastCollectionBase() { if (m_buffer) free(m_buffer); }

        uint32_t GetBucketCount() const { return m_buckets.GetCount(); }
        const int32_t* GetBuckets() const { return m_buckets.GetData(); }
        int32_t* GetBuckets() { return m_buckets.GetData(); }
        void ReserveBuckets(size_t count) { m_buckets.Validate(count); }
    public:
        void Reserve(uint32_t count)
        {
            if (m_capacity < count)
            {
                m_capacity = m_capacity == 0 ? count : Hash::ExpandPrime(count);
                ContainerHelpers::ReallocNodeValues(&m_buffer, &m_values, &m_nodes, m_count, m_capacity);
            }

            if (m_buckets.GetCount() == 0 && count > 0)
            {
                m_buckets.Validate(count);
            }
        }
    };

    template<typename TValue, typename TNode, typename THash, size_t capacity>
    struct IFastCollectionBase<TValue, TNode, THash, true, capacity> : public NoCopy
    {
    protected:
        static constexpr uint32_t m_capacity = capacity;
        int32_t m_buckets[capacity]{};
        TValue m_values[capacity]{};
        TNode m_nodes[capacity]{};
        uint32_t m_count = 0u;
        uint32_t m_collisions = 0u;
        uint32_t GetBucketCount() const { return m_capacity; }
        const int32_t* GetBuckets() const { return m_buckets; }
        int32_t* GetBuckets() { return m_buckets; }
        void ReserveBuckets(size_t count) { }
    public: 
        void Reserve(uint32_t newCount)
        {
            if (m_capacity < newCount)
            {
                throw std::exception("Fixed set capacity exceeded!");
            }
        }
    };


    template<typename TValue, bool is_fixed, size_t capacity, typename THash>
    struct IFastSet : public IFastCollectionBase<TValue, IFastSetNode, THash, is_fixed, capacity>
    {
        using TBase = IFastCollectionBase<TValue, IFastSetNode, THash, is_fixed, capacity>;
        using TBase::m_values;
        using TBase::m_nodes;
        using TBase::m_count;
        using TBase::m_collisions;
        using TBase::m_capacity;
        using Node = IFastSetNode;
        inline static THash Hash;

        IFastSet(uint32_t size) { TBase::Reserve(size); }
        IFastSet() : IFastSet(0u) {}

        constexpr uint32_t GetCount() const { return m_count; }
        constexpr uint32_t GetCapacity() const { return m_capacity; }
        constexpr const TValue* GetValues() const { return m_values; }
        constexpr const Node* GetNodes() const { return m_nodes; }
        TValue* GetValues() { return m_values; }
        Node* GetNodes() { return m_nodes; }
        ConstBufferView<TValue> GetValuesView() const { return { m_values, (size_t)m_count }; }
        BufferView<TValue> GetValuesView() { return { m_values, (size_t)m_count }; }
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
                const auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

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

        bool Add(const TValue& value, uint32_t& outIndex)
        {
            if (m_count == 0)
            {
                TBase::Reserve(1u);
            }

            const auto hash = Hash(value);
            const auto bucketIndex = GetBucketIndex(hash);
            const auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

            if (valueIndex == -1)
            {
                TBase::Reserve(m_count + 1u);
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

                ++m_collisions;
                TBase::Reserve(m_count + 1u);
                m_nodes[m_count] = Node(valueIndex);
                m_nodes[valueIndex].next = m_count;
            }

            SetValueIndexInBuckets(bucketIndex, (int32_t)m_count);
            m_values[m_count++] = value;

            // Dead code elimination should remove this for fixed versions.
            if (!is_fixed && m_collisions > TBase::GetBucketCount())
            {
                TBase::ReserveBuckets(Hash::ExpandPrime(m_collisions));
                ClearBuckets();
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
                memset(m_values, 0, sizeof(TValue) * m_count);
                memset(m_nodes, 0, sizeof(Node) * m_count);
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

    template<typename TKey, typename TValue, bool is_fixed, size_t capacity, typename THash>
    struct IFastMap : public IFastCollectionBase<TValue, IFastMapNode<TKey>, THash, is_fixed, capacity>
    {
        using TBase = IFastCollectionBase<TValue, IFastMapNode<TKey>, THash, is_fixed, capacity>;
        using TBase::m_values;
        using TBase::m_nodes;
        using TBase::m_count;
        using TBase::m_collisions;
        using TBase::m_capacity;
        using Node = IFastMapNode<TKey>;
        inline static THash Hash;

        struct KeyValues
        {
            const Node* nodes;
            TValue* values;
            size_t count;
        };

        IFastMap(uint32_t size) { TBase::Reserve(size); }
        IFastMap() : IFastMap(0u) {}

        constexpr uint32_t GetCount() const { return m_count; }
        constexpr size_t GetCapacity() const { return m_capacity; }
        constexpr const TValue* GetValues() const { return m_values; }
        constexpr const Node* GetNodes() const { return m_nodes; }
        TValue* GetValues() { return m_values; }
        Node* GetNodes() { return m_nodes; }
        ConstBufferView<TValue> GetValuesView() const { return { m_values, (size_t)m_count }; }
        BufferView<TValue> GetValuesView() { return { m_values, (size_t)m_count }; }
        KeyValues GetKeyValues() { return { m_nodes, m_values, (size_t)m_count }; }
        const TValue* GetValuePtr(const TKey& key) const { auto index = GetIndex(key); return index != -1 ? &m_values[index] : nullptr; }
        const TValue& GetValueAt(uint32_t index) const { return m_values[index]; }
        const TKey& GetKeyAt(uint32_t index) const { return m_nodes[index].key; }
        TValue* GetValuePtr(const TKey& key) { auto index = GetIndex(key); return index != -1 ? &m_values[index] : nullptr; }
        TValue& GetValueAt(uint32_t index) { return m_values[index]; }
        void SetValue(const TKey& key, const TValue& value) { auto index = GetIndex(key); if (index != -1) m_values[index] = value; }
        void SetValueAt(uint32_t index, const TValue& value) { m_values[index] = value; }

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
            if (m_count == 0)
            {
                TBase::Reserve(1u);
            }

            const auto hash = Hash(key);
            const auto bucketIndex = GetBucketIndex(hash);
            const auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

            if (valueIndex == -1)
            {
                TBase::Reserve(m_count + 1u);
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
                TBase::Reserve(m_count + 1u);
                m_nodes[m_count] = Node(key, hash, valueIndex);
                m_nodes[valueIndex].next = m_count;
            }

            SetValueIndexInBuckets(bucketIndex, m_count++);

            if (!is_fixed && m_collisions > TBase::GetBucketCount())
            {
                TBase::ReserveBuckets(Hash::ExpandPrime(m_collisions));
                ClearBuckets();
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
                memset(m_values, 0, sizeof(TValue) * m_count);
                memset(m_nodes, 0, sizeof(Node) * m_count);
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
    using FastSet = IFastSet<TValue, false, 0, THash>;

    template<typename TValue, size_t capacity, typename THash = std::hash<TValue>>
    using FixedSet = IFastSet<TValue, true, capacity, THash>;

    template<typename TValue>
    using PointerSet = IFastSet<TValue*, false, 0, Hash::TPointerHash<TValue>>;

    template<typename TValue, size_t capacity>
    using FixedPointerSet = IFastSet<TValue*, true, capacity, Hash::TPointerHash<TValue>>;


    template<typename TKey, typename TValue, typename THash = std::hash<TKey>>
    using FastMap = IFastMap<TKey, TValue, false, 0, THash>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = std::hash<TKey>>
    using FixedMap = IFastMap<TKey, TValue, true, capacity, THash>;

    template<typename TKey, typename TValue, typename THash = std::hash<TKey>>
    using PointerMap = IFastMap<TKey, TValue*, false, 0, THash>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = std::hash<TKey>>
    using FixedPointerMap = IFastMap<TKey, TValue*, true, capacity, THash>;
}
