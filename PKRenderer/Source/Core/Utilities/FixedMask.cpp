#include "PrecompiledHeader.h"
#include "Core/Platform/Platform.h"
#include "FixedMask.h"

namespace PK
{
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
    
    int64_t BinaryUtilities::FindFirstZero(const uint64_t* buffer, size_t capacity)
    {
        int64_t index = -1ll;
        auto size = (capacity + 63ull) / 64ull;

        for (auto i = 0ull; i < size; ++i)
        {
            auto bitpos = Platform::BitScan64(~buffer[i]);

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
        if (count == 1u)
        {
            return FindFirstZero(buffer, capacity);
        }

        auto base = 0ull;
        auto head = 0ull;
        auto in_scope = false;

        while (head < capacity)
        {
            if (in_scope)
            {
                const auto mask = ~((1ull << (head & 63u)) - 1ull);
                const auto bits = buffer[head >> 6ull] & mask;
                const auto pos1 = Platform::BitScan64(bits);
                head += pos1 - (head & 63u);
                in_scope = pos1 == 64u;
            }
            else
            {
                const auto mask = ((1ull << (head & 63u)) - 1ull);
                const auto bits = ~(buffer[head >> 6ull] | mask);
                const auto pos0 = Platform::BitScan64(bits);
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

    void BinaryUtilities::FlipRange(uint64_t* buffer, size_t start, size_t end)
    {
        auto head = start;

        if ((end - start) == 1ull)
        {
            buffer[start >> 6ull] ^= 1ull << (start & 63ull);
            return;
        }

        while (head < end)
        {
            auto remaining = end - (head & 0xFFFFFFFFFFFFFFC0ull);
            auto shiftbeg = head & 63ull;
            auto shiftend = 64ull - (remaining > 64ull ? 64ull : remaining);
            auto mask = ((~0ull) << shiftbeg) & ((~0ull) >> shiftend);
            buffer[head >> 6ull] ^= mask;
            head = (head + 64ull) & 0xFFFFFFFFFFFFFFC0ull;
        }
    }
}
