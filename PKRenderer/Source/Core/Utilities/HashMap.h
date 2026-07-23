#pragma once
#include "NoCopy.h"
#include "Hash.h"
#include "Memory.h"

namespace PK
{
    template<typename TIndex>
    struct IHashSetNode
    {
        using Index = TIndex;
        Index previous;
        Index next;
        IHashSetNode() : previous(static_cast<Index>(-1)), next(static_cast<Index>(-1)) {}
        IHashSetNode(Index previous) : previous(previous), next(static_cast<Index>(-1)) {}
    };

    template<typename TIndex, typename TKey>
    struct IHashMapNode
    {
        using Index = TIndex;
        using Key = TKey;
        Key key;
        size_t hashcode;
        Index previous;
        Index next;
        IHashMapNode() : key(), hashcode(0ull), previous(static_cast<Index>(-1)), next(static_cast<Index>(-1)) {}
        IHashMapNode(const Key& key, uint64_t hash) : key(key), hashcode(hash), previous(static_cast<Index>(-1)), next(static_cast<Index>(-1)) {}
    };

    template<typename TValue, typename TNode>
    struct IHashAllocatorHeap : public NoCopy
    {
    protected:
        using Index = typename TNode::Index;
        using Node = TNode;
        using Value = TValue;

        void* m_buffer = nullptr;

        union
        {
            Index* m_buckets = nullptr;
            Index m_bucketsInline;
        };

        Value* m_values = nullptr;
        Node* m_nodes = nullptr;
        uint32_t m_bucketStride = 0u;
        uint32_t m_bucketCount = 0u;
        uint32_t m_collisions = 0u;
        uint32_t m_capacity = 0u;
        uint32_t m_count = 0u;

        ~IHashAllocatorHeap()
        {
            if (m_buffer)
            {
                Memory::ClearArray(m_values, m_count);
                Memory::ClearArray(m_nodes, m_count);
                Memory::Free(m_buffer);
            }
        }

        uint32_t GetBucketCount() const { return m_bucketCount + 1u; }
        const Index* GetBuckets() const { return m_buffer ? m_buckets : &m_bucketsInline; }
        Index* GetBuckets() { return m_buffer ? m_buckets : &m_bucketsInline; }

        void Move(IHashAllocatorHeap&& other)
        {
            if (this != &other)
            {
                if (m_buffer)
                {
                    Memory::ClearArray(m_values, m_count);
                    Memory::ClearArray(m_nodes, m_count);
                    Memory::Free(m_buffer);
                }

                m_buffer = PK::Exchange(other.m_buffer, nullptr);
                m_buckets = PK::Exchange(other.m_buckets, nullptr);
                m_values = PK::Exchange(other.m_values, nullptr);
                m_nodes = PK::Exchange(other.m_nodes, nullptr);
                m_bucketStride = PK::Exchange(other.m_bucketStride, 0u);
                m_bucketCount = PK::Exchange(other.m_bucketCount, 0u);
                m_collisions = PK::Exchange(other.m_collisions, 0u);
                m_capacity = PK::Exchange(other.m_capacity, 0u);
                m_count = PK::Exchange(other.m_count, 0u);
            }
        }

        void Copy(const IHashAllocatorHeap& other)
        {
            if (m_buffer)
            {
                Memory::ClearArray(m_values, m_count);
                Memory::ClearArray(m_nodes, m_count);
                Memory::Free(m_buffer);
            }
            
            m_buffer = nullptr;
            m_capacity = 0ull;
            m_count = other.m_count;
            m_collisions = other.m_collisions;
            m_bucketStride = other.m_bucketStride;

            Reserve(other.m_capacity);
            Memory::CopyArray(m_values, other.m_values, other.m_count);
            Memory::CopyArray(m_buckets, other.m_buckets, other.m_count);
            Memory::CopyArray(m_nodes, other.m_nodes, other.m_count);
        }

    public:
        bool Reserve(uint32_t capacity)
        {
            if (m_capacity < capacity)
            {
                m_capacity = m_capacity == 0 ? capacity : (uint32_t)Hash::ExpandPrime(capacity);
                // Use offset 1 for bucket count & stride to have valid zeroed memory state without construction.
                m_bucketCount = (m_bucketStride + 1u) * m_capacity - 1u;
                
                size_t size = 0ull;
                const auto offsetBuckets = Memory::AlignSize<Index>(size);
                size = offsetBuckets + sizeof(Index) * GetBucketCount();
                const auto offsetNode = Memory::AlignSize<Node>(size);
                size = offsetNode + sizeof(Node) * m_capacity;
                const auto offsetValue = Memory::AlignSize<Value>(size);
                size = offsetValue + sizeof(Value) * m_capacity;
                
                auto newBuffer = Memory::AllocateClear<uint8_t>(size);
                auto newBuckets = Memory::CastOffsetPtr<Index>(newBuffer, offsetBuckets);
                auto newNodes = Memory::CastOffsetPtr<Node>(newBuffer, offsetNode);
                auto newValues = Memory::CastOffsetPtr<Value>(newBuffer, offsetValue);

                if (m_buffer)
                {
                    Memory::MoveArray(newValues, m_values, m_count);
                    Memory::MoveArray(newNodes, m_nodes, m_count);
                    Memory::Free(m_buffer);
                }

                m_buffer = newBuffer;
                m_values = newValues;
                m_nodes = newNodes;
                m_buckets = newBuckets;
                return true;
            }

            return false;
        }

        bool Reserve(uint32_t capacity, uint32_t bucketStride)
        {
            m_bucketStride = bucketStride > 0ull ? (bucketStride - 1ull) : 0ull;
            return Reserve(capacity);
        }
    };

    template<typename TValue, typename TNode, size_t capacity, size_t bucket_stride>
    struct IHashAllocatorFixed : public NoCopy
    {
    protected:
        using Index = typename TNode::Index;
        using Node = TNode;
        using Value = TValue;

        static constexpr uint32_t m_capacity = capacity;
        Index m_buckets[capacity * bucket_stride]{};
        Node m_nodes[capacity]{};
        Value m_values[capacity]{};
        uint32_t m_collisions = 0u;
        uint32_t m_count = 0u;

        uint32_t GetBucketCount() const { return m_capacity * bucket_stride; }
        const Index* GetBuckets() const { return m_buckets; }
        Index* GetBuckets() { return m_buckets; }

        void Move(IHashAllocatorFixed&& other)
        {
            Memory::MoveArray(m_buckets, other.m_buckets, capacity * bucket_stride);
            Memory::MoveArray(m_nodes, other.m_nodes, capacity);
            Memory::MoveArray(m_values, other.m_values, capacity);
            m_collisions = PK::Exchange(other.m_collisions, 0u);
            m_count = PK::Exchange(other.m_count, 0u);
        }

        void Copy(const IHashAllocatorFixed& other)
        {
            Memory::CopyArray(m_buckets, other.m_buckets, capacity * bucket_stride);
            Memory::CopyArray(m_nodes, other.m_nodes, capacity);
            Memory::CopyArray(m_values, other.m_values, capacity);
            m_collisions = other.m_collisions;
            m_count = other.m_count;
        }

    public: 
        bool Reserve(uint32_t newCount)
        {
            Memory::Assert(m_capacity >= newCount, "Fixed allocator capacity exceeded!");
            return false;
        }

        bool Reserve(uint32_t count, [[maybe_unused]] uint32_t bucketCountFactor)
        {
            return Reserve(count);
        }
    };


    template<typename TAllocator, typename THash>
    struct IHashSet : public TAllocator
    {
        using Base = TAllocator;
        using Index = typename Base::Index;
        using Node = typename Base::Node;
        using Value = typename Base::Value;
        inline static THash Hash;

        constexpr static const Index INVALID_INDEX = static_cast<Index>(-1);

        IHashSet(uint32_t size, uint32_t bucketCountFactor) { Base::Reserve(size, bucketCountFactor); }
        IHashSet() : IHashSet(0u, 1u) {}
        IHashSet(IHashSet&& other) noexcept { Base::Move(PK::Forward<TAllocator>(other)); }
        IHashSet(const IHashSet& other) noexcept { Base::Copy(other); }

        const Value& operator[](uint32_t index) const { return Base::m_values[index]; }
        Value& operator[](uint32_t index) { return Base::m_values[index]; }
        IHashSet& operator=(IHashSet&& other) noexcept { Base::Move(PK::Forward<TAllocator>(other)); return *this; }
        IHashSet& operator=(const IHashSet& other) noexcept { Base::Copy(other); return *this; }

        constexpr uint32_t GetCount() const { return Base::m_count; }
        constexpr uint32_t GetCapacity() const { return Base::m_capacity; }
        constexpr const Value* GetValues() const { return Base::m_values; }
        Value* GetValues() { return Base::m_values; }
        constexpr Value const* begin() const { return Base::m_values; }
        constexpr Value const* end() const { return Base::m_values + Base::m_count; }

        // Special case access where key is inlined into value.
        const Value* GetValuePtr(const Value& value) const { auto index = GetIndex(value); return index != -1 ? &Base::m_values[index] : nullptr; }
        Value* GetValuePtr(const Value& value) { auto index = GetIndex(value); return index != -1 ? &Base::m_values[index] : nullptr; }

        uint32_t GetBucketIndex(uint64_t hash) const { return (uint32_t)(hash % Base::GetBucketCount()); }
        void SetValueIndexInBuckets(uint32_t i, Index value) { Base::GetBuckets()[i] = value + 1u; }
        Index GetValueIndexFromBuckets(uint32_t i) const { return Base::GetBuckets()[i] - 1u; }

        int32_t GetHashIndex(size_t hash) const
        {
            if (Base::m_count > 0)
            {
                const auto bucketIndex = GetBucketIndex(hash);
                auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

                while (valueIndex != INVALID_INDEX)
                {
                    if (Hash(Base::m_values[valueIndex]) == hash)
                    {
                        return valueIndex;
                    }

                    valueIndex = Base::m_nodes[valueIndex].previous;
                }
            }

            return -1;
        }

        int32_t GetIndex(const Value& value) const
        {
            if (Base::m_count > 0)
            {
                const auto hash = Hash(value);
                const auto bucketIndex = GetBucketIndex(hash);
                auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

                while (valueIndex != INVALID_INDEX)
                {
                    if (Base::m_values[valueIndex] == value)
                    {
                        return valueIndex;
                    }

                    valueIndex = Base::m_nodes[valueIndex].previous;
                }
            }

            return -1;
        }

        bool Contains(const Value& value) const { return GetIndex(value) != -1; }

        bool Add(const Value& value, uint32_t* outIndex)
        {
            const auto hash = Hash(value);
            const auto bucketIndex = GetBucketIndex(hash);
            const auto valueIndex = GetValueIndexFromBuckets(bucketIndex);
            auto movingValueIndex = valueIndex;

            while (movingValueIndex != INVALID_INDEX)
            {
                if (Base::m_values[movingValueIndex] == value)
                {
                    *outIndex = movingValueIndex;
                    return false;
                }

                movingValueIndex = Base::m_nodes[movingValueIndex].previous;
            }
                
            const auto resized = Base::Reserve(Base::m_count + 1u);
            const auto index = Base::m_count++;
            Base::m_values[index] = value;

            if (!resized)
            {
                Base::m_nodes[index] = Node(valueIndex);

                if (valueIndex != INVALID_INDEX)
                {
                    Base::m_collisions++;
                    Base::m_nodes[valueIndex].next = static_cast<Index>(index);
                }

                SetValueIndexInBuckets(bucketIndex, static_cast<Index>(index));
            }
            else // Dead code elimination should remove this for fixed versions.
            {
                Base::m_collisions = 0u;

                for (auto newValueIndex = 0u; newValueIndex < Base::m_count; newValueIndex++)
                {
                    const auto existingBucketIndex = GetBucketIndex(Hash(Base::m_values[newValueIndex]));
                    const auto existingValueIndex = GetValueIndexFromBuckets(existingBucketIndex);
                    SetValueIndexInBuckets(existingBucketIndex, static_cast<Index>(newValueIndex));

                    if (existingValueIndex != INVALID_INDEX)
                    {
                        ++Base::m_collisions;
                        Base::m_nodes[newValueIndex].previous = existingValueIndex;
                        Base::m_nodes[newValueIndex].next = INVALID_INDEX;
                        Base::m_nodes[existingValueIndex].next = static_cast<Index>(newValueIndex);
                    }
                    else
                    {
                        Base::m_nodes[newValueIndex].next = INVALID_INDEX;
                        Base::m_nodes[newValueIndex].previous = INVALID_INDEX;
                    }
                }
            }

            *outIndex = index;
            return true;
        }

        uint32_t Add(const Value& value)
        {
            uint32_t outIndex = 0u;
            Add(value, &outIndex);
            return outIndex;
        }

        bool RemoveAt(uint32_t index)
        {
            if (index >= Base::m_count)
            {
                return false;
            }

            const auto hash = Hash(Base::m_values[index]);
            const auto bucketIndex = GetBucketIndex(hash);

            if (GetValueIndexFromBuckets(bucketIndex) == index)
            {
                SetValueIndexInBuckets(bucketIndex, Base::m_nodes[index].previous);
            }

            const auto updateNext = Base::m_nodes[index].next;
            const auto updatePrevious = Base::m_nodes[index].previous;

            if (updateNext != INVALID_INDEX)
            {
                Base::m_nodes[updateNext].previous = updatePrevious;
            }

            if (updatePrevious != INVALID_INDEX)
            {
                Base::m_collisions--;
                Base::m_nodes[updatePrevious].next = updateNext;
            }

            Base::m_count--;

            if (index != Base::m_count)
            {
                const auto movingBucketIndex = GetBucketIndex(Hash(Base::m_values[Base::m_count]));

                if (GetValueIndexFromBuckets(movingBucketIndex) == Base::m_count)
                {
                    SetValueIndexInBuckets(movingBucketIndex, index);
                }

                const auto next = Base::m_nodes[Base::m_count].next;
                const auto previous = Base::m_nodes[Base::m_count].previous;

                if (next != INVALID_INDEX)
                {
                    Base::m_nodes[next].previous = index;
                }

                if (previous != INVALID_INDEX)
                {
                    Base::m_nodes[previous].next = index;
                }

                Base::m_nodes[index] = Base::m_nodes[Base::m_count];
                Base::m_values[index] = PK::MoveTemp(Base::m_values[Base::m_count]);
            }

            return true;
        }

        bool Remove(const Value& value)
        {
            auto index = GetIndex(value);

            if (index != -1)
            {
                return RemoveAt(index);
            }

            return index != -1;
        }

        void ClearFast()
        {
            if (Base::m_count > 0)
            {
                Memory::Memset<Index>(Base::GetBuckets(), 0, Base::GetBucketCount());
                Base::m_collisions = 0u;
                Base::m_count = 0u;
            }
        }

        void Clear()
        {
            if (Base::m_count > 0)
            {
                Memory::ClearArray(Base::m_values, Base::m_count);
                Memory::ClearArray(Base::m_nodes, Base::m_count);
                ClearFast();
            }
        }
    };

    template<typename TAllocator, typename THash>
    struct IHashMap : public TAllocator
    {
        using Base = TAllocator;
        using Index = typename Base::Index;
        using Node = typename Base::Node;
        using Value = typename Base::Value;
        using Key = typename Base::Node::Key;
        inline static THash Hash;

        constexpr static const Index INVALID_INDEX = static_cast<Index>(-1);

        struct KeyValueConst
        {
            const Key& key;
            const Value& value;
            KeyValueConst(const Key& key, const Value& value) : key(key), value(value) {}
        };

        struct KeyValue
        {
            Key& key;
            Value& value;
            KeyValue(Key& key, Value& value) : key(key), value(value){}
        };

        IHashMap(uint32_t size, uint32_t bucketCountFactor) { Base::Reserve(size, bucketCountFactor); }
        IHashMap() : IHashMap(0u, 1u) {}
        IHashMap(IHashMap&& other) noexcept { Base::Move(PK::Forward<Base>(other)); }
        IHashMap(const IHashMap& other) noexcept { Base::Copy(other); }

        const KeyValueConst operator[](uint32_t index) const { return { Base::m_nodes[index].key, Base::m_values[index] }; }
        KeyValue operator[](uint32_t index) { return { Base::m_nodes[index].key, Base::m_values[index] }; }
        IHashMap& operator=(IHashMap&& other) noexcept { Base::Move(PK::Forward<Base>(other)); return *this; }
        IHashMap& operator=(const IHashMap& other) noexcept { Base::Copy(other); return *this; }

        constexpr uint32_t GetCount() const { return Base::m_count; }
        constexpr size_t GetCapacity() const { return Base::m_capacity; }
        constexpr const Value* GetValues() const { return Base::m_values; }
        Value* GetValues() { return Base::m_values; }
        constexpr Value const* begin() const { return Base::m_values; }
        constexpr Value const* end() const { return Base::m_values + Base::m_count; }

        const Value* GetValuePtr(const Key& key) const { auto index = GetIndex(key); return index != -1 ? &Base::m_values[index] : nullptr; }
        Value* GetValuePtr(const Key& key) { auto index = GetIndex(key); return index != -1 ? &Base::m_values[index] : nullptr; }
        void SetValue(const Key& key, const Value& value) { auto index = GetIndex(key); if (index != -1) Base::m_values[index] = value; }

        uint32_t GetBucketIndex(uint64_t hash) const { return (uint32_t)(hash % Base::GetBucketCount()); }
        void SetValueIndexInBuckets(uint32_t i, Index value) { Base::GetBuckets()[i] = value + 1u; }
        Index GetValueIndexFromBuckets(uint32_t i) const { return Base::GetBuckets()[i] - 1u; }

        int32_t GetIndex(const Key& key) const
        {
            if (Base::m_count > 0)
            {
                const auto hash = Hash(key);
                const auto bucketIndex = GetBucketIndex(hash);
                auto valueIndex = GetValueIndexFromBuckets(bucketIndex);

                while (valueIndex != INVALID_INDEX)
                {
                    if (Base::m_nodes[valueIndex].hashcode == hash && 
                        Base::m_nodes[valueIndex].key == key)
                    {
                        return valueIndex;
                    }

                    valueIndex = Base::m_nodes[valueIndex].previous;
                }
            }

            return -1;
        }

        bool Contains(const Key& key) const { return GetIndex(key) != -1; }

        bool AddKey(const Key& key, uint32_t* outIndex)
        {
            const auto hash = Hash(key);
            const auto bucketIndex = GetBucketIndex(hash);
            const auto valueIndex = GetValueIndexFromBuckets(bucketIndex);
            auto movingValueIndex = valueIndex;

            while (movingValueIndex != INVALID_INDEX)
            {
                if (Base::m_nodes[movingValueIndex].hashcode == hash && 
                    Base::m_nodes[movingValueIndex].key == key)
                {
                    *outIndex = movingValueIndex;
                    return false;
                }

                movingValueIndex = Base::m_nodes[movingValueIndex].previous;
            }

            const auto resized = Base::Reserve(Base::m_count + 1u);
            const auto index = Base::m_count++;
            Base::m_nodes[index] = Node(key, hash);

            if (!resized)
            {
                Base::m_nodes[index].previous = valueIndex;

                if (valueIndex != INVALID_INDEX)
                {
                    Base::m_collisions++;
                    Base::m_nodes[valueIndex].next = static_cast<Index>(index);
                }

                SetValueIndexInBuckets(bucketIndex, static_cast<Index>(index));
            }
            else // Dead code elimination should remove this for fixed versions.
            {
                Base::m_collisions = 0;

                for (auto newValueIndex = 0u; newValueIndex < Base::m_count; newValueIndex++)
                {
                    const auto existingBucketIndex = GetBucketIndex(Base::m_nodes[newValueIndex].hashcode);
                    const auto existingValueIndex = GetValueIndexFromBuckets(existingBucketIndex);
                    SetValueIndexInBuckets(existingBucketIndex, static_cast<Index>(newValueIndex));

                    if (existingValueIndex != INVALID_INDEX)
                    {
                        Base::m_collisions++;
                        Base::m_nodes[newValueIndex].previous = existingValueIndex;
                        Base::m_nodes[newValueIndex].next = INVALID_INDEX;
                        Base::m_nodes[existingValueIndex].next = static_cast<Index>(newValueIndex);
                    }
                    else
                    {
                        Base::m_nodes[newValueIndex].next = INVALID_INDEX;
                        Base::m_nodes[newValueIndex].previous = INVALID_INDEX;
                    }
                }
            }

            *outIndex = index;
            return true;
        }

        uint32_t AddKey(const Key& key)
        {
            uint32_t newIndex = 0u;
            AddKey(key, &newIndex);
            return newIndex;
        }

        bool AddValue(const Key& key, const Value& value)
        {
            auto index = 0u;
            auto appended = AddKey(key, &index);
            Base::m_values[index] = value;
            return appended;
        }

        bool RemoveAt(uint32_t index)
        {
            if (index >= Base::m_count)
            {
                return false;
            }

            const auto bucketIndex = GetBucketIndex(Base::m_nodes[index].hashcode);

            if (GetValueIndexFromBuckets(bucketIndex) == index)
            {
                SetValueIndexInBuckets(bucketIndex, Base::m_nodes[index].previous);
            }

            const auto updateNext = Base::m_nodes[index].next;
            const auto updatePrevious = Base::m_nodes[index].previous;

            if (updateNext != INVALID_INDEX)
            {
                Base::m_nodes[updateNext].previous = updatePrevious;
            }

            if (updatePrevious != INVALID_INDEX)
            {
                Base::m_collisions--;
                Base::m_nodes[updatePrevious].next = updateNext;
            }

            Base::m_count--;

            if (index != Base::m_count)
            {
                const auto movingBucketIndex = GetBucketIndex(Base::m_nodes[Base::m_count].hashcode);

                if (GetValueIndexFromBuckets(movingBucketIndex) == Base::m_count)
                {
                    SetValueIndexInBuckets(movingBucketIndex, static_cast<Index>(index));
                }

                const auto next = Base::m_nodes[Base::m_count].next;
                const auto previous = Base::m_nodes[Base::m_count].previous;

                if (next != INVALID_INDEX)
                {
                    Base::m_nodes[next].previous = static_cast<Index>(index);
                }

                if (previous != INVALID_INDEX)
                {
                    Base::m_nodes[previous].next = static_cast<Index>(index);
                }

                Base::m_nodes[index] = Base::m_nodes[Base::m_count];
                Base::m_values[index] = PK::MoveTemp(Base::m_values[Base::m_count]);
            }

            return true;
        }

        bool Remove(const Key& key)
        {
            auto index = GetIndex(key);

            if (index != -1)
            {
                return RemoveAt(index);
            }

            return index != -1;
        }

        void ClearFast()
        {
            if (Base::m_count > 0)
            {
                Memory::Memset<Index>(Base::GetBuckets(), 0, Base::GetBucketCount());
                Base::m_collisions = 0u;
                Base::m_count = 0u;
            }
        }

        void Clear()
        {
            if (Base::m_count > 0)
            {
                Memory::ClearArray(Base::m_values, Base::m_count);
                Memory::ClearArray(Base::m_nodes, Base::m_count);
                ClearFast();
            }
        }
    };


    template<typename TValue, typename THash = Hash::THash<TValue>>
    using HashSet = IHashSet<IHashAllocatorHeap<TValue, IHashSetNode<uint32_t>>, THash>;

    template<typename TValue, typename THash = Hash::THash<TValue>>
    using HashSet16 = IHashSet<IHashAllocatorHeap<TValue, IHashSetNode<uint16_t>>, THash>;

    template<typename TValue, size_t capacity, typename THash = Hash::THash<TValue>, size_t bucket_stride = 1ull>
    using FixedSet = IHashSet<IHashAllocatorFixed<TValue, IHashSetNode<uint32_t>, capacity, bucket_stride>, THash>;

    template<typename TValue, size_t capacity, typename THash = Hash::THash<TValue>, size_t bucket_stride = 1ull>
    using FixedSet8 = IHashSet<IHashAllocatorFixed<TValue, IHashSetNode<uint8_t>, capacity, bucket_stride>, THash>;

    template<typename TValue, size_t capacity, typename THash = Hash::THash<TValue>, size_t bucket_stride = 1ull>
    using FixedSet16 = IHashSet<IHashAllocatorFixed<TValue, IHashSetNode<uint16_t>, capacity, bucket_stride>, THash>;


    template<typename TKey, typename TValue, typename THash = Hash::THash<TKey>>
    using HashMap = IHashMap<IHashAllocatorHeap<TValue, IHashMapNode<uint32_t, TKey>>, THash>;

    template<typename TKey, typename TValue, typename THash = Hash::THash<TKey>>
    using HashMap16 = IHashMap<IHashAllocatorHeap<TValue, IHashMapNode<uint16_t, TKey>>, THash>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = Hash::THash<TKey>, size_t bucket_stride = 1ull>
    using FixedMap = IHashMap<IHashAllocatorFixed<TValue, IHashMapNode<uint32_t, TKey>, capacity, bucket_stride>, THash>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = Hash::THash<TKey>, size_t bucket_stride = 1ull>
    using FixedMap8 = IHashMap<IHashAllocatorFixed<TValue, IHashMapNode<uint8_t, TKey>, capacity, bucket_stride>, THash>;

    template<typename TKey, typename TValue, size_t capacity, typename THash = Hash::THash<TKey>, size_t bucket_stride = 1ull>
    using FixedMap16 = IHashMap<IHashAllocatorFixed<TValue, IHashMapNode<uint16_t, TKey>, capacity, bucket_stride>, THash>;
}
