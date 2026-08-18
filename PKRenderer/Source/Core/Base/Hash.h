#pragma once
#include "Types/UUID128.h"
#include "Templates.h"

namespace PK::Hash
{
    uint32_t ExpandPrime(uint32_t oldSize);
    uint32_t ExpandSize(uint32_t capacity, uint32_t size);
    inline size_t ExpandPrime(size_t oldSize) { return ExpandPrime((uint32_t)oldSize); }
    inline size_t ExpandSize(size_t capacity, size_t size) { return ExpandSize((uint32_t)capacity, (uint32_t)size); }

    uint32_t ByteArrayHash(const void* data, size_t size);
    uint64_t MurmurHash(const void* data, size_t size, uint64_t seed);
    uint64_t FNV1AHash(const void* data, size_t size);
    uint64_t InterlaceHash32x2(uint32_t a, uint32_t b);

    // Declared here so that we can use consteval
    consteval UUID128 MurmurHash128(const char* data, size_t size) noexcept
    {
        const auto seed = 18446744073709551557ull;
        const auto blockCount = size / 16ull;

        uint64_t h1 = seed;
        uint64_t h2 = seed;
        uint64_t c1 = 0x87c37b91114253d5ull;
        uint64_t c2 = 0x4cf5ad432745937full;

        for (auto i = 0ull; i < blockCount; ++i)
        {
            const char* current_block = data + (i * 16ull);

            uint64_t k1 = 0;
            uint64_t k2 = 0;

            for (int b = 0; b < 8; ++b)
            {
                k1 |= static_cast<uint64_t>(static_cast<uint8_t>(current_block[b])) << (b * 8);
                k2 |= static_cast<uint64_t>(static_cast<uint8_t>(current_block[b + 8])) << (b * 8);
            }

            k1 *= c1; 
            k1 = (k1 << 31) | (k1 >> (64 - 31));
            k1 *= c2; 
            h1 ^= k1;

            h1 = (h1 << 27) | (h1 >> (64 - 27)); 
            h1 += h2; 
            h1 = h1 * 5 + 0x52dce729;

            k2 *= c2; 
            k2 = (k2 << 33) | (k2 >> (64 - 33)); 
            k2 *= c1; 
            h2 ^= k2;

            h2 = (h2 << 31) | (h2 >> (64 - 31));
            h2 += h1; 
            h2 = h2 * 5 + 0x38495ab5;
        }

        const char* tail = data + blockCount * 16u;

        uint64_t k1 = 0ull;
        uint64_t k2 = 0ull;

        switch (size & 15)
        {
            case 15: k2 ^= uint64_t(static_cast<uint8_t>(tail[14])) << 48;
            case 14: k2 ^= uint64_t(static_cast<uint8_t>(tail[13])) << 40;
            case 13: k2 ^= uint64_t(static_cast<uint8_t>(tail[12])) << 32;
            case 12: k2 ^= uint64_t(static_cast<uint8_t>(tail[11])) << 24;
            case 11: k2 ^= uint64_t(static_cast<uint8_t>(tail[10])) << 16;
            case 10: k2 ^= uint64_t(static_cast<uint8_t>(tail[9])) << 8;
            case  9: k2 ^= uint64_t(static_cast<uint8_t>(tail[8])) << 0;
                k2 *= c2; 
                k2 = (k2 << 33) | (k2 >> (64 - 33)); 
                k2 *= c1; h2 ^= k2;

            case  8: k1 ^= uint64_t(static_cast<uint8_t>(tail[7])) << 56;
            case  7: k1 ^= uint64_t(static_cast<uint8_t>(tail[6])) << 48;
            case  6: k1 ^= uint64_t(static_cast<uint8_t>(tail[5])) << 40;
            case  5: k1 ^= uint64_t(static_cast<uint8_t>(tail[4])) << 32;
            case  4: k1 ^= uint64_t(static_cast<uint8_t>(tail[3])) << 24;
            case  3: k1 ^= uint64_t(static_cast<uint8_t>(tail[2])) << 16;
            case  2: k1 ^= uint64_t(static_cast<uint8_t>(tail[1])) << 8;
            case  1: k1 ^= uint64_t(static_cast<uint8_t>(tail[0])) << 0;
                k1 *= c1; 
                k1 = (k1 << 31) | (k1 >> (64 - 31));
                k1 *= c2; 
                h1 ^= k1;
        };

        h1 ^= size; 
        h2 ^= size;
        h1 += h2;
        h2 += h1;

        // fmix
        {
            h1 ^= h1 >> 33;
            h1 *= 0xff51afd7ed558ccdull;
            h1 ^= h1 >> 33;
            h1 *= 0xc4ceb9fe1a85ec53ull;
            h1 ^= h1 >> 33;
        }

        // fmix
        {
            h2 ^= h2 >> 33;
            h2 *= 0xff51afd7ed558ccdull;
            h2 ^= h2 >> 33;
            h2 *= 0xc4ceb9fe1a85ec53ull;
            h2 ^= h2 >> 33;
        }

        h1 += h2;
        h2 += h1;

        UUID128 uuid;
        uuid.low = h1;
        uuid.high = h2;
        return uuid;
    }

    template<typename T>
    inline size_t GetHash(const T& k) 
    {
        if constexpr (TIsPointer<T>)
        {
            // Reduce collisions by reducing type ptr offsets to a minimum.
            return reinterpret_cast<size_t>(k) / sizeof(typename TRemovePtr<T>::Type); 
        } 
        else if constexpr (TIsEnum<T>)
        {
            return (__underlying_type(T))k;
        }
        else
        {
            return FNV1AHash(&k, sizeof(T));
        }
    }

    template<> inline size_t GetHash(const uint8_t& k) { return k; }
    template<> inline size_t GetHash(const uint16_t& k) { return k; }
    template<> inline size_t GetHash(const uint32_t& k) { return k; }
    template<> inline size_t GetHash(const uint64_t& k) { return k; }
    
    template<> inline size_t GetHash(const int8_t& k) { return static_cast<unsigned>(k); }
    template<> inline size_t GetHash(const int16_t& k) { return static_cast<unsigned>(k); }
    template<> inline size_t GetHash(const int32_t& k) { return static_cast<unsigned>(k); }
    template<> inline size_t GetHash(const int64_t& k) { return static_cast<unsigned>(k); }

    template<> inline size_t GetHash(const float& k) { return *reinterpret_cast<const uint32_t*>(&k); }
    template<> inline size_t GetHash(const double& k) { return *reinterpret_cast<const uint64_t*>(&k); }

    template<typename T> struct TMurmurHash { size_t operator()(const T& k) const noexcept { return Hash::MurmurHash(&k, sizeof(T), 18446744073709551557ull); } };
    template<typename T> struct TFNV1AHash  { size_t operator()(const T& k) const noexcept { return Hash::FNV1AHash(&k, sizeof(T)); } };
    template<typename T> struct TCastHash { size_t operator()(const T& k) const noexcept { return static_cast<size_t>(k); } };
    template<typename T> struct TPointerHash { size_t operator()(const T* k) const noexcept { return reinterpret_cast<size_t>(k) / sizeof(T); } };
    template<typename T> struct THash { size_t operator()(const T& k) const noexcept { return GetHash<T>(k); } };
}
