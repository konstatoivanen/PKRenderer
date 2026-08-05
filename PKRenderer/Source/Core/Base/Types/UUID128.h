#pragma once
#include <stdint.h>

namespace PK
{
    struct alignas(16) UUID128
    {
        union
        {
            #if defined(__clang__)
            __uint128_t value;
            #endif
            struct
            {
                uint64_t low;
                uint64_t high;
            };

            uint8_t bytes[16];
        };
    };

    #if defined(__clang__)
    constexpr bool operator==(const UUID128& a, const UUID128& b) { return a.value == b.value; }
    constexpr bool operator!=(const UUID128& a, const UUID128& b) { return a.value != b.value; }
    constexpr bool operator< (const UUID128& a, const UUID128& b) { return a.value < b.value; }
    constexpr bool operator> (const UUID128& a, const UUID128& b) { return a.value > b.value; }
    constexpr bool operator<=(const UUID128& a, const UUID128& b) { return a.value <= b.value; }
    constexpr bool operator>=(const UUID128& a, const UUID128& b) { return a.value >= b.value; }
    #else
    constexpr bool operator==(const UUID128& a, const UUID128& b) { return a.high == b.high && a.low == b.low; }
    constexpr bool operator!=(const UUID128& a, const UUID128& b) { return a.high != b.high || a.low != b.low; }
    constexpr bool operator< (const UUID128& a, const UUID128& b) { return a.high != b.high ? a.high < b.high : a.low < b.low; }
    constexpr bool operator> (const UUID128& a, const UUID128& b) { return a.high != b.high ? a.high > b.high : a.low > b.low; }
    constexpr bool operator<=(const UUID128& a, const UUID128& b) { return a.high != b.high ? a.high <= b.high : a.low <= b.low; }
    constexpr bool operator>=(const UUID128& a, const UUID128& b) { return a.high != b.high ? a.high >= b.high : a.low >= b.low; }
    #endif
}