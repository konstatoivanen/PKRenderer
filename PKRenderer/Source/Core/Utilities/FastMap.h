#pragma once
#include <limits>
#include "Hash.h"
#include "NoCopy.h"
#include "BufferView.h"
#include "BufferIterator.h"
#include "ContainerHelpers.h"

namespace PK
{
    template<typename TIndex>
    struct IFastSetNode
    {
        using _TIndex = TIndex;
        TIndex previous;
        TIndex next;
        IFastSetNode() : previous(-1), next(-1) {}
        IFastSetNode(TIndex previous) : previous(previous), next(-1) {}
    };

    template<typename TIndex, typename TKey>
    struct IFastMapNode
    {
        using _TIndex = TIndex;
        using _TKey = TKey;
        TKey key;
        size_t hashcode;
        TIndex previous;
        TIndex next;
        IFastMapNode() : key(), hashcode(0ull), previous(-1), next(-1) {}
        IFastMapNode(const TKey& key, uint64_t hash) : key(key), hashcode(hash), previous(-1), next(-1) {}
    };

    template<typename TValue, typename TNode>
    struct IFastCollectionAllocatorDynamic : public NoCopy
    {
    protected:
        using _TIndex = typename TNode::_TIndex;
        using _TNode = TNode;
        using _TValue = TValue;

        void* m_buffer = nullptr;

        union
        {
            _TIndex* m_buckets = nullptr;
            _TIndex m_bucketsInline;
        };

        _TValue* m_values = nullptr;
        _TNode* m_nodes = nullptr;
        uint32_t m_bucketCountFactor = 1u;
        uint32_t m_bucketCount = 1u;
        uint32_t m_collisions = 0u;
        uint32_t m_capacity = 0u;
        uint32_t m_count = 0u;
        
        ~IFastCollectionAllocatorDynamic()
        {
            if (m_buffer)
            {
                ContainerHelpers::ClearArray(m_values, m_count);
                ContainerHelpers::ClearArray(m_nodes, m_count);
                free(m_buffer);
            }
        }

        uint32_t GetBucketCount() const { return m_bucketCount; }
        const _TIndex* GetBuckets() const { return m_buffer ? m_buckets : &m_bucketsInline; }
        _TIndex* GetBuckets() { return m_buffer ? m_buckets : &m_bucketsInline; }

    public:
        bool Reserve(uint32_t count)
        {
            if (m_capacity < count)
            {
                m_capacity = m_capacity == 0 ? count : Hash::ExpandPrime(count);
                m_bucketCount = m_bucketCountFactor * m_capacity;
                
                size_t size = 0ull;
                const auto offsetBuckets = ContainerHelpers::AlignSize<_TIndex>(&size);
                size += sizeof(_TIndex) * m_bucketCount;
                const auto offsetNode = ContainerHelpers::AlignSize<_TNode>(&size);
                size += sizeof(_TNode) * m_capacity;
                const auto offsetValue = ContainerHelpers::AlignSize<_TValue>(&size);
                size += sizeof(_TValue) * m_capacity;
                
                auto newBuffer = calloc(size, 1u);
                auto newBuckets = ContainerHelpers::CastOffsetPtr<_TIndex>(newBuffer, offsetBuckets);
                auto newNodes = ContainerHelpers::CastOffsetPtr<_TNode>(newBuffer, offsetNode);
                auto newValues = ContainerHelpers::CastOffsetPtr<_TValue>(newBuffer, offsetValue);

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

    template<typename TValue, typename TNode, size_t capacity, size_t bucket_count_factor>
    struct IFastCollectionAllocatorFixed : public NoCopy
    {
    protected:
        using _TIndex = typename TNode::_TIndex;
        using _TNode = TNode;
        using _TValue = TValue;

        static constexpr uint32_t m_capacity = capacity;
        _TIndex m_buckets[capacity * bucket_count_factor]{};
        _TNode m_nodes[capacity]{};
        _TValue m_values[capacity]{};
        uint32_t m_collisions = 0u;
        uint32_t m_count = 0u;

        uint32_t GetBucketCount() const { return m_capacity * bucket_count_factor; }
        const _TIndex* GetBuckets() const { return m_buckets; }
        _TIndex* GetBuckets() { return m_buckets; }

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


    template<typename TAllocator, typename THash>
    struct IFastSet : public TAllocator
    {
        using TBase = TAllocator;
        using TIndex = typename TBase::_TIndex;
        using TNode = typename TBase::_TNode;
        using TValue = typename TBase::_TValue;
        inline static THash Hash;

        IFastSet(uint32_t size, uint32_t bucketCountFactor) { TBase::Reserve(size, bucketCountFactor); }
        IFastSet() : IFastSet(0u, 1u) {}

        constexpr uint32_t GetCount() const { return TBase::m_count; }
        constexpr uint32_t GetCapacity() const { return TBase::m_capacity; }
        constexpr const TValue* GetValues() const { return TBase::m_values; }
        TValue* GetValues() { return m_values; }
        ConstBufferIterator<TValue> begin() const { return ConstBufferIterator<TValue>(TBase::m_values, 0ull); }
        ConstBufferIterator<TValue> end() const { return ConstBufferIterator<TValue>(TBase::m_values + TBase::m_count, TBase::m_count); }
        const TValue& operator[](uint32_t index) const { return TBase::m_values[index]; }
        TValue& operator[](uint32_t index) { return TBase::m_values[index]; }

        // Special case access where key is inlined into value.
        const TValue* GetValuePtr(const TValue& value) const { auto index = GetIndex(value); return index != -1 ? &TBase::m_values[index] : nullptr; }
        TValue* GetValuePtr(const TValue& value) { auto index = GetIndex(value); return index != -1 ? &TBase::m_values[index] : nullptr; }

        int32_t GetHashIndex(size_t hash) const
        {
            if (TBase::m_count > 0)
            {
                const auto bucketIndex = GetBucketIndex(hash);
                auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

                while (valueIndex != -1)
                {
                    if (Hash(TBase::m_values[valueIndex]) == hash)
                    {
                        return valueIndex;
                    }

                    valueIndex = TBase::m_nodes[valueIndex].previous;
                }
            }

            return -1;
        }

        int32_t GetIndex(const TValue& value) const
        {
            if (TBase::m_count > 0)
            {
                const auto hash = Hash(value);
                const auto bucketIndex = GetBucketIndex(hash);
                auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

                while (valueIndex != -1)
                {
                    if (TBase::m_values[valueIndex] == value)
                    {
                        return valueIndex;
                    }

                    valueIndex = TBase::m_nodes[valueIndex].previous;
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
                if (TBase::m_values[movingValueIndex] == value)
                {
                    *outIndex = movingValueIndex;
                    return false;
                }

                movingValueIndex = TBase::m_nodes[movingValueIndex].previous;
            }
                
            const auto resized = TBase::Reserve(TBase::m_count + 1u);
            const auto index = TBase::m_count++;
            TBase::m_values[index] = value;

            if (!resized)
            {
                TBase::m_nodes[index] = TNode(valueIndex);

                if (valueIndex != -1)
                {
                    TBase::m_collisions++;
                    TBase::m_nodes[valueIndex].next = index;
                }

                SetValueIndexInBuckets(bucketIndex, index);
            }
            else // Dead code elimination should remove this for fixed versions.
            {
                TBase::m_collisions = 0u;

                for (auto newValueIndex = 0u; newValueIndex < TBase::m_count; newValueIndex++)
                {
                    const auto existingBucketIndex = GetBucketIndex(Hash(TBase::m_values[newValueIndex]));
                    const auto existingValueIndex = GetValueIndexFromBuckets(existingBucketIndex);
                    SetValueIndexInBuckets(existingBucketIndex, newValueIndex);

                    if (existingValueIndex != -1)
                    {
                        ++TBase::m_collisions;
                        TBase::m_nodes[newValueIndex].previous = existingValueIndex;
                        TBase::m_nodes[newValueIndex].next = -1;
                        TBase::m_nodes[existingValueIndex].next = newValueIndex;
                    }
                    else
                    {
                        TBase::m_nodes[newValueIndex].next = -1;
                        TBase::m_nodes[newValueIndex].previous = -1;
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
            if (index >= TBase::m_count)
            {
                return false;
            }

            const auto hash = Hash(TBase::m_values[index]);
            const auto bucketIndex = GetBucketIndex(hash);

            if (GetValueIndexFromBuckets(bucketIndex) == (int32_t)index)
            {
                SetValueIndexInBuckets(bucketIndex, TBase::m_nodes[index].previous);
            }

            const auto updateNext = TBase::m_nodes[index].next;
            const auto updatePrevious = TBase::m_nodes[index].previous;

            if (updateNext != -1)
            {
                TBase::m_nodes[updateNext].previous = updatePrevious;
            }

            if (updatePrevious != -1)
            {
                TBase::m_collisions--;
                TBase::m_nodes[updatePrevious].next = updateNext;
            }

            TBase::m_count--;

            if (index != TBase::m_count)
            {
                const auto movingBucketIndex = GetBucketIndex(Hash(TBase::m_values[TBase::m_count]));

                if (GetValueIndexFromBuckets(movingBucketIndex) == (int32_t)TBase::m_count)
                {
                    SetValueIndexInBuckets(movingBucketIndex, index);
                }

                const auto next = TBase::m_nodes[TBase::m_count].next;
                const auto previous = TBase::m_nodes[TBase::m_count].previous;

                if (next != -1)
                {
                    TBase::m_nodes[next].previous = index;
                }

                if (previous != -1)
                {
                    TBase::m_nodes[previous].next = index;
                }

                TBase::m_nodes[index] = TBase::m_nodes[m_count];
                TBase::m_values[index] = TBase::m_values[m_count];
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
            if (TBase::m_count > 0)
            {
                ContainerHelpers::ClearArray(TBase::m_values, TBase::m_count);
                ContainerHelpers::ClearArray(TBase::m_nodes, TBase::m_count);
                ClearBuckets();
                TBase::m_collisions = 0u;
                TBase::m_count = 0u;
            }
        }

        void ClearFast()
        {
            if (TBase::m_count > 0)
            {
                ClearBuckets();
                TBase::m_collisions = 0u;
                TBase::m_count = 0u;
            }
        }

    private:
        uint32_t GetBucketIndex(uint64_t hash) const { return (uint32_t)(hash % TBase::GetBucketCount()); }
        void SetValueIndexInBuckets(uint32_t i, int32_t value) { TBase::GetBuckets()[i] = value + 1; }
        int32_t GetValueIndexFromBuckets(uint32_t i) const { return TBase::GetBuckets()[i] - 1; }
        void ClearBuckets() { memset(TBase::GetBuckets(), 0, sizeof(TIndex) * TBase::GetBucketCount()); }
    };

    template<typename TAllocator, typename THash>
    struct IFastMap : public TAllocator
    {
        using TBase = TAllocator;
        using TNode = typename TBase::_TNode;
        using TValue = typename TBase::_TValue;
        using TKey = typename TBase::_TNode::_TKey;
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

        constexpr uint32_t GetCount() const { return TBase::m_count; }
        constexpr size_t GetCapacity() const { return TBase::m_capacity; }
        constexpr const TValue* GetValues() const { return TBase::m_values; }
        TValue* GetValues() { return TBase::m_values; }
        ConstBufferIterator<TValue> begin() const { return ConstBufferIterator<TValue>(TBase::m_values, 0ull); }
        ConstBufferIterator<TValue> end() const { return ConstBufferIterator<TValue>(TBase::m_values + TBase::m_count, TBase::m_count); }
        const KeyValueConst operator[](uint32_t index) const { return { TBase::m_nodes[index].key, TBase::m_values[index] }; }
        KeyValue operator[](uint32_t index) { return { TBase::m_nodes[index].key, TBase::m_values[index] }; }

        const TValue* GetValuePtr(const TKey& key) const { auto index = GetIndex(key); return index != -1 ? &TBase::m_values[index] : nullptr; }
        TValue* GetValuePtr(const TKey& key) { auto index = GetIndex(key); return index != -1 ? &TBase::m_values[index] : nullptr; }
        void SetValue(const TKey& key, const TValue& value) { auto index = GetIndex(key); if (index != -1) TBase::m_values[index] = value; }

        int32_t GetIndex(const TKey& key) const
        {
            if (TBase::m_count > 0)
            {
                const auto hash = Hash(key);
                const auto bucketIndex = GetBucketIndex(hash);
                auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

                while (valueIndex != -1)
                {
                    if (TBase::m_nodes[valueIndex].hashcode == hash && TBase::m_nodes[valueIndex].key == key)
                    {
                        return valueIndex;
                    }

                    valueIndex = TBase::m_nodes[valueIndex].previous;
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
                if (TBase::m_nodes[movingValueIndex].hashcode == hash && TBase::m_nodes[movingValueIndex].key == key)
                {
                    *outIndex = movingValueIndex;
                    return false;
                }

                movingValueIndex = TBase::m_nodes[movingValueIndex].previous;
            }

            const auto resized = TBase::Reserve(TBase::m_count + 1u);
            const auto index = TBase::m_count++;
            TBase::m_nodes[index] = TNode(key, hash);

            if (!resized)
            {
                TBase::m_nodes[index].previous = valueIndex;

                if (valueIndex != -1)
                {
                    TBase::m_collisions++;
                    TBase::m_nodes[valueIndex].next = index;
                }

                SetValueIndexInBuckets(bucketIndex, index);
            }
            else // Dead code elimination should remove this for fixed versions.
            {
                TBase::m_collisions = 0;

                for (auto newValueIndex = 0u; newValueIndex < TBase::m_count; newValueIndex++)
                {
                    const auto existingBucketIndex = GetBucketIndex(TBase::m_nodes[newValueIndex].hashcode);
                    const auto existingValueIndex = GetValueIndexFromBuckets(existingBucketIndex);
                    SetValueIndexInBuckets(existingBucketIndex, newValueIndex);

                    if (existingValueIndex != -1)
                    {
                        TBase::m_collisions++;
                        TBase::m_nodes[newValueIndex].previous = existingValueIndex;
                        TBase::m_nodes[newValueIndex].next = -1;
                        TBase::m_nodes[existingValueIndex].next = newValueIndex;
                    }
                    else
                    {
                        TBase::m_nodes[newValueIndex].next = -1;
                        TBase::m_nodes[newValueIndex].previous = -1;
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
            TBase::m_values[index] = value;
            return appended;
        }

        bool RemoveAt(uint32_t index)
        {
            if (index >= TBase::m_count)
            {
                return false;
            }

            const auto bucketIndex = GetBucketIndex(TBase::m_nodes[index].hashcode);

            if (GetValueIndexFromBuckets(bucketIndex) == (int32_t)index)
            {
                SetValueIndexInBuckets(bucketIndex, TBase::m_nodes[index].previous);
            }

            const auto updateNext = TBase::m_nodes[index].next;
            const auto updatePrevious = TBase::m_nodes[index].previous;

            if (updateNext != -1)
            {
                TBase::m_nodes[updateNext].previous = updatePrevious;
            }

            if (updatePrevious != -1)
            {
                TBase::m_collisions--;
                TBase::m_nodes[updatePrevious].next = updateNext;
            }

            TBase::m_count--;

            if (index != TBase::m_count)
            {
                const auto movingBucketIndex = GetBucketIndex(TBase::m_nodes[TBase::m_count].hashcode);

                if (GetValueIndexFromBuckets(movingBucketIndex) == (int32_t)TBase::m_count)
                {
                    SetValueIndexInBuckets(movingBucketIndex, index);
                }

                const auto next = TBase::m_nodes[TBase::m_count].next;
                const auto previous = TBase::m_nodes[TBase::m_count].previous;

                if (next != -1)
                {
                    TBase::m_nodes[next].previous = index;
                }

                if (previous != -1)
                {
                    TBase::m_nodes[previous].next = index;
                }

                TBase::m_nodes[index] = TBase::m_nodes[TBase::m_count];
                TBase::m_values[index] = std::move(TBase::m_values[TBase::m_count]);
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
            if (TBase::m_count > 0)
            {
                ContainerHelpers::ClearArray(TBase::m_values, TBase::m_count);
                ContainerHelpers::ClearArray(TBase::m_nodes, TBase::m_count);
                ClearBuckets();
                TBase::m_collisions = 0u;
                TBase::m_count = 0u;
            }
        }

        void ClearFast()
        {
            if (TBase::m_count > 0)
            {
                ClearBuckets();
                TBase::m_collisions = 0u;
                TBase::m_count = 0u;
            }
        }

        private:
            uint32_t GetBucketIndex(uint64_t hash) const { return (uint32_t)(hash % TBase::GetBucketCount()); }
            void SetValueIndexInBuckets(uint32_t i, int32_t value) { TBase::GetBuckets()[i] = value + 1; }
            int32_t GetValueIndexFromBuckets(uint32_t i) const { return TBase::GetBuckets()[i] - 1; }
            void ClearBuckets() { memset(TBase::GetBuckets(), 0, sizeof(int32_t) * TBase::GetBucketCount()); }
    };


    template<typename TValue, typename THash = std::hash<TValue>>
    using FastSet = IFastSet<IFastCollectionAllocatorDynamic<TValue, IFastSetNode<int32_t>>, THash>;

    template<typename TValue>
    using PointerSet = IFastSet<IFastCollectionAllocatorDynamic<TValue*, IFastSetNode<int32_t>>, Hash::TPointerHash<TValue>>;

    template<typename TKey, typename TValue, typename THash = std::hash<TKey>>
    using FastMap = IFastMap<IFastCollectionAllocatorDynamic<TValue, IFastMapNode<int32_t, TKey>>, THash>;

    template<typename TKey, typename TValue, typename THash = std::hash<TKey>>
    using PointerMap = IFastMap<IFastCollectionAllocatorDynamic<TValue*, IFastMapNode<int32_t, TKey>>, THash>;


    template<typename TValue, typename THash = std::hash<TValue>>
    using FastSet16 = IFastSet<IFastCollectionAllocatorDynamic<TValue, IFastSetNode<int16_t>>, THash>;

    template<typename TValue>
    using PointerSet16 = IFastSet<IFastCollectionAllocatorDynamic<TValue*, IFastSetNode<int16_t>>, Hash::TPointerHash<TValue>>;

    template<typename TKey, typename TValue, typename THash = std::hash<TKey>>
    using FastMap16 = IFastMap<IFastCollectionAllocatorDynamic<TValue, IFastMapNode<int16_t, TKey>>, THash>;

    template<typename TKey, typename TValue, typename THash = std::hash<TKey>>
    using PointerMap16 = IFastMap<IFastCollectionAllocatorDynamic<TValue*, IFastMapNode<int16_t, TKey>>, THash>;


    template<typename TValue, size_t capacity, typename THash = std::hash<TValue>, size_t bucket_count_factor = 1ull>
    using FixedSet = IFastSet<IFastCollectionAllocatorFixed<TValue, IFastSetNode<int32_t>, capacity, bucket_count_factor>, THash>;

    template<typename TValue, size_t capacity, size_t bucket_count_factor = 1ull>
    using FixedPointerSet = IFastSet<IFastCollectionAllocatorFixed<TValue*, IFastSetNode<int32_t>, capacity, bucket_count_factor>, Hash::TPointerHash<TValue>>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = std::hash<TKey>, size_t bucket_count_factor = 1ull>
    using FixedMap = IFastMap<IFastCollectionAllocatorFixed<TValue, IFastMapNode<int32_t, TKey>, capacity, bucket_count_factor>, THash>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = std::hash<TKey>, size_t bucket_count_factor = 1ull>
    using FixedPointerMap = IFastMap<IFastCollectionAllocatorFixed<TValue*, IFastMapNode<int32_t, TKey>, capacity, bucket_count_factor>, THash>;


    template<typename TValue, size_t capacity, typename THash = std::hash<TValue>, size_t bucket_count_factor = 1ull>
    using FixedSet16 = IFastSet<IFastCollectionAllocatorFixed<TValue, IFastSetNode<int16_t>, capacity, bucket_count_factor>, THash>;

    template<typename TValue, size_t capacity, size_t bucket_count_factor = 1ull>
    using FixedPointerSet16 = IFastSet<IFastCollectionAllocatorFixed<TValue*, IFastSetNode<int16_t>, capacity, bucket_count_factor>, Hash::TPointerHash<TValue>>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = std::hash<TKey>, size_t bucket_count_factor = 1ull>
    using FixedMap16 = IFastMap<IFastCollectionAllocatorFixed<TValue, IFastMapNode<int16_t, TKey>, capacity, bucket_count_factor>, THash>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = std::hash<TKey>, size_t bucket_count_factor = 1ull>
    using FixedPointerMap16 = IFastMap<IFastCollectionAllocatorFixed<TValue*, IFastMapNode<int16_t, TKey>, capacity, bucket_count_factor>, THash>;


    template<typename TValue, size_t capacity, typename THash = std::hash<TValue>, size_t bucket_count_factor = 1ull>
    using FixedSet8 = IFastSet<IFastCollectionAllocatorFixed<TValue, IFastSetNode<int8_t>, capacity, bucket_count_factor>, THash>;

    template<typename TValue, size_t capacity, size_t bucket_count_factor = 1ull>
    using FixedPointerSet8 = IFastSet<IFastCollectionAllocatorFixed<TValue*, IFastSetNode<int8_t>, capacity, bucket_count_factor>, Hash::TPointerHash<TValue>>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = std::hash<TKey>, size_t bucket_count_factor = 1ull>
    using FixedMap8 = IFastMap<IFastCollectionAllocatorFixed<TValue, IFastMapNode<int8_t, TKey>, capacity, bucket_count_factor>, THash>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = std::hash<TKey>, size_t bucket_count_factor = 1ull>
    using FixedPointerMap8 = IFastMap<IFastCollectionAllocatorFixed<TValue*, IFastMapNode<int8_t, TKey>, capacity, bucket_count_factor>, THash>;
}
