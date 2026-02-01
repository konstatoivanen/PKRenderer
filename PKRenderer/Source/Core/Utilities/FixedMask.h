#pragma once
#include <intrin.h>
#include <cstdint>

namespace PK
{
    namespace BinaryUtilities
    {
        size_t CountBits(const uint64_t* buffer, size_t size);
        int64_t FindFirstZero(const uint64_t* buffer, size_t size, size_t capacity);
        int64_t FindFirstZeroRange(const uint64_t* buffer, size_t capacity, uint32_t count);
    }

    template<size_t capacity>
    struct FixedMask
    {
        constexpr static size_t Capacity = capacity;
        constexpr static size_t Stride = 64ull;
        constexpr static size_t Size = ((capacity + 63ull) / 64ull);
        uint64_t m_mask[Size]{};
        size_t CountBits() const { return BinaryUtilities::CountBits(m_mask, Size); }
        int64_t FindFirstZero() const { return BinaryUtilities::FindFirstZero(m_mask, Size, Capacity); }
        int64_t FindFirstZeroRange(uint32_t count) const { return BinaryUtilities::FindFirstZeroRange(m_mask, Capacity, count); }
        bool GetAt(size_t index) const { return (m_mask[index >> 6ull] & (1ull << (index & 63ull))) != 0u; }
        void SetAt(size_t index, bool value) { m_mask[index >> 6ull] = (m_mask[index >> 6ull] & ~(1ull << (index & 63ull))) | ((uint64_t)value << (index & 63u)); }
        void FlipAt(size_t index) { m_mask[index >> 6ull] ^= 1ull << (index & 63ull); }
        void Clear() { memset(m_mask, 0, sizeof(m_mask)); }
    };
}
