#pragma once
#include <stdint.h>

namespace PK::Hash
{
    constexpr static const int32_t MaxPrimeArrayLength = 0x7FEFFFFD;
    constexpr static const int32_t HashPrime = 101;
    constexpr static const int32_t PrimesLength = 72;
    constexpr static const int32_t Primes[PrimesLength] = {
        3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
        1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591,
        17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363, 156437,
        187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897, 1162687, 1395263,
        1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369};

    bool IsPrime(int32_t candidate);
    int32_t GetPrime(int32_t min);
    uint32_t ExpandPrime(uint32_t oldSize);
    uint32_t ExpandSize(uint32_t capacity, uint32_t size);

    uint32_t ByteArrayHash(const void* data, size_t count);
    uint64_t MurmurHash(const void* data, size_t count, uint64_t seed);
    uint64_t FNV1AHash(const void* data, size_t count);

    // Note morton order performed poorly in comparison due to too regular offsets.
    // This works ok in cases where both a & b are linear low order values.
    inline uint64_t InterlaceHash32x2(uint32_t a, uint32_t b)
    {
        uint64_t o = 0ull;
        o |= ((uint64_t)a & 0x000000FFull) << 0ull;
        o |= ((uint64_t)b & 0x000000FFull) << 8ull;
        o |= ((uint64_t)a & 0x0000FF00ull) << 8ull;
        o |= ((uint64_t)b & 0x0000FF00ull) << 16ull;
        o |= ((uint64_t)a & 0x00FF0000ull) << 16ull;
        o |= ((uint64_t)b & 0x00FF0000ull) << 24ull;
        o |= ((uint64_t)a & 0xFF000000ull) << 24ull;
        o |= ((uint64_t)b & 0xFF000000ull) << 32ull;
        return o;
    }

    template<typename T>
    struct TMurmurHash
    {
        std::size_t operator()(const T& k) const noexcept
        {
            constexpr uint64_t seed = 18446744073709551557ull;
            return Hash::MurmurHash(&k, sizeof(T), seed);
        }
    };

    template<typename T>
    struct TFNV1AHash
    {
        size_t operator()(const T& k) const noexcept
        {
            return Hash::FNV1AHash(&k, sizeof(T));
        }
    };

    template<typename T>
    struct TPointerHash
    {
        size_t operator()(const T* k) const noexcept
        {
            // Reduce collisions by reducing type ptr offsets to a minimum.
            return reinterpret_cast<size_t>(k) / sizeof(T);
        }
    };

    template<typename T>
    struct TCastHash
    {
        size_t operator()(const T& k) const noexcept
        {
            return (size_t)(k);
        }
    };
}
