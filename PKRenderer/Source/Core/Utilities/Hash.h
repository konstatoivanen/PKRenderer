#pragma once
#include <stdint.h>

namespace PK::Hash
{
    bool IsPrime(int32_t candidate);
    int32_t GetPrime(int32_t min);
    uint32_t ExpandPrime(uint32_t oldSize);
    uint32_t ExpandSize(uint32_t capacity, uint32_t size);

    uint32_t ByteArrayHash(const void* data, size_t count);
    uint64_t MurmurHash(const void* data, size_t count, uint64_t seed);
    uint64_t FNV1AHash(const void* data, size_t count);
    uint64_t InterlaceHash32x2(uint32_t a, uint32_t b);

    template<typename T>
    struct TMurmurHash
    {
        size_t operator()(const T& k) const noexcept
        {
            return Hash::MurmurHash(&k, sizeof(T), 18446744073709551557ull);
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
