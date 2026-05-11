#pragma once
#include "NoCopy.h"
#include "Allocation.h"

namespace PK
{
    namespace BinaryUtilities
    {
        size_t CountBits(const uint64_t* buffer, size_t size);
        int64_t FindFirstZero(const uint64_t* buffer, size_t capacity);
        int64_t FindFirstZeroRange(const uint64_t* buffer, size_t capacity, uint32_t count);
        void FlipRange(uint64_t* buffer, size_t start, size_t end);
    }

    template<typename TAllocation>
    struct Mask : NoCopy
    {
        using TData = typename TAllocation::template Data<uint64_t>;
        using TMask = Mask<TAllocation>;

         // skip 64 bits if current 64 bit block is empty.
        struct ConstIterator
        {
            constexpr ConstIterator(TMask const* mask, size_t index) : m_mask(mask), m_index(index) {}
            operator bool() const noexcept { return m_mask->GetAt(m_index); }
            const ConstIterator& operator*() const { return *this; }
            constexpr size_t index() const noexcept { return m_index; }
            bool operator != (const ConstIterator& iterator) const { return m_mask != iterator.m_mask || m_index != iterator.m_index; }
            ConstIterator operator++() { ++m_index; m_index = m_mask->GetData()[m_index >> 6ull] ? m_index : ((m_index >> 6ull) + 1ull) << 6ull; return *this; }
            TMask const* m_mask;
            size_t m_index;
        };

        struct Reference 
        {
            Reference(TMask& mask, size_t index) : m_mask(&mask), m_index(index) {}
            Reference() noexcept : m_mask(nullptr), m_index(0ull) {}
            ~Reference() noexcept {}
            Reference& operator=(bool value) noexcept { m_mask->SetAt(m_index, value); return *this; }
            Reference& operator=(const Reference& ref) noexcept { m_mask->SetAt(m_index, static_cast<bool>(ref)); return *this; }
            bool operator~() const noexcept { return !m_mask->GetAt(m_index); }
            operator bool() const noexcept { return m_mask->GetAt(m_index); }
            Reference& Flip() noexcept { m_mask->FlipAt(m_index); return *this; }
            bool Exchange(bool value) noexcept { auto e = m_mask->GetAt(m_index); m_mask->SetAt(m_index, value); return e; }
            TMask* m_mask;
            size_t m_index;
        };

        constexpr Mask() : m_data() {}
        Mask(size_t capacity) noexcept : Mask() { Reserve(capacity, false); }
        Mask(Mask&& other) noexcept : Mask() { Move(PK::Forward<Mask>(other)); }
        Mask(const Mask& other) noexcept : Mask() { Copy(other); }

        constexpr size_t GetCapacity() const { return TData::GetSize(m_data) * 8ull; }
        constexpr size_t GetBlockCount() const { return TData::GetCount(m_data); }
        constexpr uint64_t* GetData() { return TData::GetPtr(m_data); }
        constexpr const uint64_t* GetData() const { return TData::GetPtr(m_data); }
        constexpr ConstIterator begin() const { return ConstIterator(this, 0ull); }
        constexpr ConstIterator end() const { return ConstIterator(this, GetCapacity()); }

        constexpr bool operator[](size_t i) const { return GetAt(i); }
        Reference operator[](size_t i) { return Reference(*this, i); }
        Mask& operator=(Mask&& other) noexcept { Move(PK::Forward<Mask>(other)); return *this; }
        Mask& operator=(const Mask& other) noexcept { Copy(other); return *this; }

        inline void Copy(const Mask& other)
        {
            Reserve(other.GetCapacity(), false);
            Memory::Memcpy<uint64_t>(GetData(), other.GetData(), GetBlockCount());
        }

        inline void Move(Mask&& other)
        {
            if (this != &other)
            {
                TData::Free(m_data);
                m_data = PK::Exchange(other.m_data, {});
            }
        }

        bool Reserve(size_t newCapacity, bool preserve)
        {
            if (newCapacity > GetCapacity())
            {
                auto newData = TData::Allocate((newCapacity + 63ull) / 64ull);

                if (preserve && GetCapacity() > 0u)
                {
                    Memory::Memcpy<uint64_t>(TData::GetPtr(newData), TData::GetPtr(m_data), GetBlockCount());
                }

                TData::Free(m_data);
                m_data = newData;
                return true;
            }

            return false;
        }

        size_t CountBits() const { return BinaryUtilities::CountBits(GetData(), GetBlockCount()); }
        int64_t FindFirstZero() const { return BinaryUtilities::FindFirstZero(GetData(), GetCapacity()); }
        int64_t FindFirstZeroRange(uint32_t count) const { return BinaryUtilities::FindFirstZeroRange(GetData(), GetCapacity(), count); }
        constexpr bool GetAt(size_t index) const { return (GetData()[index >> 6ull] & (1ull << (index & 63ull))) != 0u; }
        void SetAt(size_t index, bool value) { GetData()[index >> 6ull] = (GetData()[index >> 6ull] & ~(1ull << (index & 63ull))) | ((uint64_t)value << (index & 63u)); }
        void FlipAt(size_t index) { GetData()[index >> 6ull] ^= 1ull << (index & 63ull); }
        void FlipRange(size_t start, size_t end) { BinaryUtilities::FlipRange(GetData(), start, end); }
        void SetAll(bool value) { Memory::Memset(GetData(), value ? -1 : 0, GetBlockCount()); }

        TData m_data;
    };


    template<size_t capacity>
    using FixedMask = Mask<AllocationFixed<(capacity + 63ull) / 64ull>>;

    template<size_t inline_capacity>
    using InlineMask = Mask<AllocationInline<(inline_capacity + 63ull) / 64ull>>;

    using HeapMask = Mask<AllocationHeap>;
}
