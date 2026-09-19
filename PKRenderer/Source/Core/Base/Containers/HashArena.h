#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Allocation.h"

namespace PK
{
    template<typename TAllocation, typename TMap>
    struct HashArena : public NoCopy
    {
        using TData = typename TAllocation::template Data<uint8_t>;

        struct AllocationHeader
        {
            uint64_t hash;
            uint32_t size;
            uint32_t accessed;
        };

        constexpr HashArena() : m_map(), m_data(), m_head(0ull) {}
        
        HashArena(size_t capacity, size_t blockCapacity) noexcept : HashArena() 
        { 
            Reserve(capacity, false); 
            m_map.Reserve(blockCapacity);
        }

        ~HashArena() { TData::Free(m_data); }

        constexpr size_t GetAllocationCount() const { return m_map.GetCount(); }
        constexpr size_t GetSize() const { return m_head; }
        constexpr size_t GetCapacity() const { return TData::GetSize(m_data); }

        bool Reserve(size_t newSize, bool preserve)
        {
            if (newSize > GetCapacity())
            {
                auto newData = TData::Allocate(newSize);

                if (preserve && GetSize() > 0u)
                {
                    Memory::Memcpy(TData::GetPtr(newData), TData::GetPtr(m_data), GetSize());
                }

                TData::Free(m_data);
                m_data = newData;
                return true;
            }

            return false;
        }

        void* Allocate(uint64_t hash, uint32_t size)
        {
            auto alignedSize = (size + sizeof(AllocationHeader) + 15ull) & ~15ull;
            
            auto index = 0u;
            auto isNew = m_map.AddKey(hash, &index);
            auto offset = isNew ? m_head : m_map[index].value;

            if (!isNew && GetHeader(offset)->size < alignedSize)
            {
                GetHeader(offset)->hash = 0ull;
                GetHeader(offset)->accessed = 0u;
                offset = m_head;
            }

            Reserve(offset + alignedSize, true);
            auto header = GetHeader(offset);
            header->hash = hash;
            header->accessed = 1u;
            header->size = (uint32_t)alignedSize;
            m_map[index].value = offset;
            m_head = m_head >= offset + alignedSize ? m_head : offset + alignedSize;
            return TData::GetPtr(m_data) + offset + sizeof(AllocationHeader);
        }

        void Prune()
        {
            m_map.ClearFast();
            const auto size = m_head;
            auto data = TData::GetPtr(m_data);
            auto copyHead = 0ull;
            m_head = 0ull;

            while (copyHead < size)
            {
                auto header = *GetHeader(copyHead);

                if (header.accessed && m_head < copyHead)
                {
                    Memory::Memmove(data + m_head, data + copyHead, header.size);
                }

                if (header.accessed)
                {
                    m_map.AddValue(header.hash, m_head);
                    GetHeader(m_head)->accessed = 0u;
                    m_head += header.size;
                }

                copyHead += header.size;
            }

            if (m_head < GetCapacity())
            {
                Memory::Memset(data + m_head, 0, GetCapacity() - m_head);
            }
        }

    private:
        AllocationHeader* GetHeader(size_t offset)
        {
            return reinterpret_cast<AllocationHeader*>(TData::GetPtr(m_data) + offset);
        }

        HashMap<uint64_t, size_t> m_map;
        TData m_data;
        size_t m_head;
    };

    template<size_t capacity, size_t blockCapacity>
    using FixedHashArena = HashArena<AllocationFixed<capacity>, FixedMap<uint64_t, size_t, blockCapacity>>;

    using HeapHashArena = HashArena<AllocationHeap, HashMap<uint64_t, size_t>>;
}
