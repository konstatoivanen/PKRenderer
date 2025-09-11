#pragma once
#include <intrin.h>

namespace PK
{
    template<size_t capacity>
    struct FixedMask
    {
        constexpr static size_t Capacity = capacity;
        constexpr static size_t Stride = sizeof(uint64_t) * 8ull;
        constexpr static size_t Size = (capacity / Stride) + 1ull;
        uint64_t m_mask[Size]{};

        bool GetAt(size_t index) const
        {
            auto base = index / Stride;
            auto bit = 1ull << (index - base * Stride);
            return (m_mask[base] & bit) != 0u;
        }

        void SetAt(size_t index, bool value)
        {
            auto base = index / Stride;
            auto bit = 1ull << (index - base * Stride);
            m_mask[base] = value ? (m_mask[base] | bit) : (m_mask[base] & ~bit);
        }

        void ToggleAt(size_t index)
        {
            auto base = index / Stride;
            m_mask[base] ^= 1ull << (index - base * Stride);
        }

        void Clear() { memset(m_mask, 0, sizeof(m_mask)); }

        size_t CountBits() const
        {
            auto count = 0ull;

            for (size_t i = 0ull; i < Size; ++i)
            {
                auto j = m_mask[i];
                j = j - ((j >> 1) & 0x5555555555555555);
                j = (j & 0x3333333333333333) + ((j >> 2) & 0x3333333333333333);
                j = (((j + (j >> 4)) & 0xF0F0F0F0F0F0F0F) * 0x101010101010101) >> 56;
                count += j;
            }

            return count;
        }

        int64_t FirstFalse() const
        {
            for (auto i = 0ull; i < Size; ++i)
            {
// @TODO this check is nonsense.
#if defined(_MSC_VER) && defined(_WIN64)
                unsigned long idx = 0ul;
                if (_BitScanForward64(&idx, ~m_mask[i]))
                {
                    return i * Stride + idx;
                }

#elif defined __GNUC__ || defined __clang__
                auto idx = __builtin_ffsll(~m_mask[i])) - 1ull;
     
                if (idx >= 0)
                {
                    return i * Stride + idx;
                }
#endif
            }

            return -1;
        }
    };
}
