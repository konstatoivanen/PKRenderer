#pragma once
#include <stdint.h>

namespace PK
{
    namespace BinaryUtilities
    {
        size_t CountBits(const uint64_t* buffer, size_t size);
        int64_t FindFirstZero(const uint64_t* buffer, size_t capacity);
        int64_t FindFirstZeroRange(const uint64_t* buffer, size_t capacity, uint32_t count);
        void FlipRange(uint64_t* buffer, size_t start, size_t end);
    }

    template<size_t capacity>
    struct FixedMask
    {
        using TFixedMask = FixedMask<capacity>;

        struct Reference 
        {
            Reference(TFixedMask& mask, size_t index) : mask(&mask), index(index) {}
            Reference() noexcept : mask(nullptr), index(0ull) {}
            ~Reference() noexcept {}
            Reference& operator=(bool value) noexcept { mask->SetAt(index, value); return *this; }
            Reference& operator=(const Reference& ref) noexcept { mask->SetAt(index, static_cast<bool>(ref)); return *this; }
            bool operator~() const noexcept { return !mask->GetAt(index); }
            operator bool() const noexcept { return mask->GetAt(index); }
            Reference& Flip() noexcept { mask->FlipAt(index); return *this; }
            bool Exchange(bool value) noexcept { auto e = mask->GetAt(index); mask->SetAt(index, value); return e; }
            TFixedMask* mask;
            size_t index;
        };

        constexpr static size_t Capacity = capacity;
        constexpr static size_t Stride = 64ull;
        constexpr static size_t Size = ((capacity + 63ull) / 64ull);
        size_t CountBits() const { return BinaryUtilities::CountBits(m_mask, Size); }
        int64_t FindFirstZero() const { return BinaryUtilities::FindFirstZero(m_mask, Capacity); }
        int64_t FindFirstZeroRange(uint32_t count) const { return BinaryUtilities::FindFirstZeroRange(m_mask, Capacity, count); }
        constexpr bool GetAt(size_t index) const { return (m_mask[index >> 6ull] & (1ull << (index & 63ull))) != 0u; }
        void SetAt(size_t index, bool value) { m_mask[index >> 6ull] = (m_mask[index >> 6ull] & ~(1ull << (index & 63ull))) | ((uint64_t)value << (index & 63u)); }
        void FlipAt(size_t index) { m_mask[index >> 6ull] ^= 1ull << (index & 63ull); }
        void FlipRange(size_t start, size_t end) { BinaryUtilities::FlipRange(m_mask, start, end); }
        void SetAll(bool value) { memset(m_mask, value ? -1 : 0, sizeof(m_mask)); }
        constexpr bool operator[](size_t i) const { return GetAt(i); }
        Reference operator[](size_t i) { return Reference(*this, i); }
        uint64_t m_mask[Size]{};
    };
}
