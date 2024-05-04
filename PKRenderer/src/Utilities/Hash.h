#pragma once
#include <stdint.h>

namespace PK::Utilities::Hash
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
    int32_t ExpandPrime(uint32_t oldSize);

    uint32_t ByteArrayHash(const void* data, size_t count);
    uint64_t MurmurHash(const void* data, size_t count, uint64_t seed);
    uint64_t FNV1AHash(const void* data, size_t count);

    template<typename T>
    struct TMurmurHash
    {
        std::size_t operator()(const T& k) const noexcept
        {
            constexpr uint64_t seed = 18446744073709551557;
            return PK::Utilities::Hash::MurmurHash(&k, sizeof(T), seed);
        }
    };

    template<typename T>
    struct TFNV1AHash
    {
        size_t operator()(const T& k) const noexcept
        {
            return PK::Utilities::Hash::FNV1AHash(&k, sizeof(T));
        }
    };
}