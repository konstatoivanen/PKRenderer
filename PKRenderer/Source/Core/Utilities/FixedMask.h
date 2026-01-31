#pragma once
#include <intrin.h>
#include <cstdint>

namespace PK
{
    template<size_t capacity>
    struct FixedMask
    {
        constexpr static size_t Capacity = capacity;
        constexpr static size_t Stride = 64ull;
        constexpr static size_t Size = ((capacity + 63ull) / 64ull);
        uint64_t m_mask[Size]{};

        inline static uint32_t BitScan64(uint64_t mask)
        {
#if defined(_MSC_VER) && defined(_WIN64)
            unsigned long index = 0ul;
            return _BitScanForward64(&index, mask) ? index : 64u;
#elif defined(__GNUC__)
            auto index = __builtin_ffsll(mask);
            return index == 0 ? 64u : (uint32_t)(index - 1);
#endif
        }

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

        int64_t FindFirstZero() const
        {
            for (auto i = 0ull; i < Size; ++i)
            {
                auto index = BitScan64(~m_mask[i]);
                
                if (index < 64u)
                {
                    return i * 64ull + index;
                }
            }

            return -1;
        }

        int64_t FindFirstZeroRange(uint32_t count) const
        {
            auto base = 0ull;
            auto head = 0ull;
            auto in_scope = false;

            while (head < Capacity)
            {
                if (in_scope)
                {
                    const auto mask = ~((1ull << (head & 63u)) - 1ull);
                    const auto bits = m_mask[head >> 6ull] & mask;
                    const auto pos1 = BitScan64(bits);
                    head += pos1 - (head & 63u);
                    in_scope = pos1 == 64u; 
                }
                else
                {
                    const auto mask = ((1ull << (head & 63u)) - 1ull);
                    const auto bits = ~(m_mask[head >> 6ull] | mask);
                    const auto pos0 = BitScan64(bits);
                    head += pos0 - (head & 63u);
                    base = head; 
                    in_scope = pos0 != 64u; 
                }

                if (head - base >= count)
                {
                    return base;
                }
            }

            return -1;
        }

        bool GetAt(size_t index) const { return (m_mask[index >> 6ull] & (1ull << (index & 63ull))) != 0u; }
        void SetAt(size_t index, bool value) { m_mask[index >> 6ull] = (m_mask[index >> 6ull] & ~(1ull << (index & 63ull))) | ((uint64_t)value << (index & 63u)); }
        void FlipAt(size_t index) { m_mask[index >> 6ull] ^= 1ull << (index & 63ull); }
        void Clear() { memset(m_mask, 0, sizeof(m_mask)); }
    };
}
