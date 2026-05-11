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

        struct Reference 
        {
            Reference(TMask& mask, size_t index) : mask(&mask), index(index) {}
            Reference() noexcept : mask(nullptr), index(0ull) {}
            ~Reference() noexcept {}
            Reference& operator=(bool value) noexcept { mask->SetAt(index, value); return *this; }
            Reference& operator=(const Reference& ref) noexcept { mask->SetAt(index, static_cast<bool>(ref)); return *this; }
            bool operator~() const noexcept { return !mask->GetAt(index); }
            operator bool() const noexcept { return mask->GetAt(index); }
            Reference& Flip() noexcept { mask->FlipAt(index); return *this; }
            bool Exchange(bool value) noexcept { auto e = mask->GetAt(index); mask->SetAt(index, value); return e; }
            TMask* mask;
            size_t index;
        };

        constexpr Mask() : m_data() {}
        Mask(size_t capacity) noexcept : Mask() { Reserve(capacity); }
        Mask(Mask&& other) noexcept : Mask() { Move(PK::Forward<Mask>(other)); }
        Mask(const Mask& other) noexcept : Mask() { Copy(other.GetData(), other.GetCount()); }

        constexpr size_t GetCapacity() const { return TAllocation::GetSize(m_data) * 8ull; }
        constexpr size_t GetBlockCount() const { return TAllocation::GetCount(m_data); }
        constexpr uint64_t* GetData() { return TAllocation::GetPtr(m_data); }
        constexpr const uint64_t* GetData() const { return TAllocation::GetPtr(m_data); }

        constexpr bool operator[](size_t i) const { return GetAt(i); }
        Reference operator[](size_t i) { return Reference(*this, i); }
        Mask& operator=(Mask&& other) noexcept { Move(PK::Forward<Mask>(other)); return *this; }
        Mask& operator=(const Mask& other) noexcept { Copy(other); return *this; }

        inline void Copy(const Mask& other)
        {
            Reserve(other.GetCapacity());
            Memory::Memcpy<uint64_t>(GetData(), other.GetData(), GetBlockCount());
        }

        inline void Move(Mask&& other)
        {
            if (this != &other)
            {
                TAllocation::Free(m_data);
                m_data = PK::Exchange(other.m_data, {});
            }
        }

        bool Reserve(size_t newCapacity)
        {
            if (newCapacity <= GetCapacity())
            {
                return false;
            }

            auto newData = TAllocation::template Allocate<uint64_t>((newCapacity + 63ull) / 64ull);

            if (GetCapacity() > 0u)
            {
                Memory::Memcpy<uint64_t>(TAllocation::GetPtr(newData), TAllocation::GetPtr(m_data), GetBlockCount());
            }

            TAllocation::Free(m_data);
            m_data = newData;
            return true;
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
