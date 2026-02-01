#include "PrecompiledHeader.h"
#include "FixedMask.h"

namespace PK
{
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

    size_t BinaryUtilities::CountBits(const uint64_t* buffer, size_t size)
    {
        auto count = 0ull;

        for (size_t i = 0ull; i < size; ++i)
        {
            auto j = buffer[i];
            j = j - ((j >> 1) & 0x5555555555555555);
            j = (j & 0x3333333333333333) + ((j >> 2) & 0x3333333333333333);
            j = (((j + (j >> 4)) & 0xF0F0F0F0F0F0F0F) * 0x101010101010101) >> 56;
            count += j;
        }

        return count;
    }
    
    int64_t BinaryUtilities::FindFirstZero(const uint64_t* buffer, size_t size, size_t capacity)
    {
        int64_t index = -1ll;

        for (auto i = 0ull; i < size; ++i)
        {
            auto bitpos = BitScan64(~buffer[i]);

            if (bitpos < 64u)
            {
                index = i * 64ull + bitpos;
                break;
            }
        }

        return index < (int64_t)capacity ? index : -1ll;
    }
    
    int64_t BinaryUtilities::FindFirstZeroRange(const uint64_t* buffer, size_t capacity, uint32_t count)
    {
        auto base = 0ull;
        auto head = 0ull;
        auto in_scope = false;

        while (head < capacity)
        {
            if (in_scope)
            {
                const auto mask = ~((1ull << (head & 63u)) - 1ull);
                const auto bits = buffer[head >> 6ull] & mask;
                const auto pos1 = BitScan64(bits);
                head += pos1 - (head & 63u);
                in_scope = pos1 == 64u;
            }
            else
            {
                const auto mask = ((1ull << (head & 63u)) - 1ull);
                const auto bits = ~(buffer[head >> 6ull] | mask);
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
}
