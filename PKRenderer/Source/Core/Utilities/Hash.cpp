#include "PrecompiledHeader.h"
#include "Hash.h"

namespace PK::Hash
{
    constexpr static const uint32_t MaxPrimeArrayLength = 0x7FEFFFFDu;
    constexpr static const uint32_t HashPrime = 101u;
    constexpr static const uint32_t PrimesLength = 72u;
    constexpr static const uint32_t Primes[PrimesLength] = {
        3u, 7u, 11u, 17u, 23u, 29u, 37u, 47u, 59u, 71u, 89u, 107u, 131u, 163u, 197u, 239u, 293u, 353u, 431u, 521u, 631u, 761u, 919u,
        1103u, 1327u, 1597u, 1931u, 2333u, 2801u, 3371u, 4049u, 4861u, 5839u, 7013u, 8419u, 10103u, 12143u, 14591u,
        17519u, 21023u, 25229u, 30293u, 36353u, 43627u, 52361u, 62851u, 75431u, 90523u, 108631u, 130363u, 156437u,
        187751u, 225307u, 270371u, 324449u, 389357u, 467237u, 560689u, 672827u, 807403u, 968897u, 1162687u, 1395263u,
        1674319u, 2009191u, 2411033u, 2893249u, 3471899u, 4166287u, 4999559u, 5999471u, 7199369u };

    static bool IsPrime(uint32_t candidate)
    {
        if ((candidate & 1u) != 0u)
        {
            auto limit = (uint32_t)sqrt(candidate);

            for (uint32_t divisor = 3u; divisor <= limit; divisor += 2u)
            {
                if ((candidate % divisor) == 0u)
                {
                    return false;
                }
            }

            return true;
        }

        return candidate == 2u;
    }

    static uint32_t GetPrime(uint32_t min)
    {
        if (min == 0u)
        {
            return 0u;
        }

        for (uint32_t i = 0u; i < PrimesLength; ++i)
        {
            uint32_t prime = Primes[i];

            if (prime >= min)
            {
                return prime;
            }
        }

        const auto maxvalue = 2147483647ull;

        for (uint32_t i = (min | 1u); i < maxvalue; i += 2u)
        {
            if (IsPrime(i) && ((i - 1u) % HashPrime != 0u))
            {
                return i;
            }
        }

        return min;
    }

    uint32_t ExpandPrime(uint32_t oldSize)
    {
        uint32_t newSize = 2u * oldSize;
        return newSize > MaxPrimeArrayLength && MaxPrimeArrayLength > oldSize ? MaxPrimeArrayLength : GetPrime(newSize);
    }

    uint32_t ExpandSize(uint32_t capacity, uint32_t size)
    {
        if (size <= capacity)
        {
            return capacity;
        }

        return ExpandPrime(size);
    }

    uint32_t ByteArrayHash(const void* data, size_t count)
    {
        const char* bytes = static_cast<const char*>(data);

        // Source: https://stackoverflow.com/questions/16340/how-do-i-generate-a-hashcode-from-a-byte-array-in-c
        const auto p = 16777619;
        auto hash = 2166136261;

        for (auto i = 0u; i < count; ++i)
        {
            hash = (hash ^ bytes[i]) * p;
        }

        hash += hash << 13;
        hash ^= hash >> 7;
        hash += hash << 3;
        hash ^= hash >> 17;
        hash += hash << 5;
        return (uint32_t)hash;
    }

    uint64_t MurmurHash(const void* key, size_t len, uint64_t seed)
    {
        const uint32_t m = 0x5bd1e995;
        const int r = 24;

        uint32_t h1 = ((uint32_t)seed) ^ (uint32_t)len;
        uint32_t h2 = ((uint32_t)(seed >> 32));

        const uint32_t* data = (const uint32_t*)key;

        while (len >= 8)
        {
            uint32_t k1 = *data++;
            k1 *= m; k1 ^= k1 >> r; k1 *= m;
            h1 *= m; h1 ^= k1;
            len -= 4;

            uint32_t k2 = *data++;
            k2 *= m; k2 ^= k2 >> r; k2 *= m;
            h2 *= m; h2 ^= k2;
            len -= 4;
        }

        if (len >= 4)
        {
            uint32_t k1 = *data++;
            k1 *= m; k1 ^= k1 >> r; k1 *= m;
            h1 *= m; h1 ^= k1;
            len -= 4;
        }

        switch (len)
        {
            case 3: h2 ^= ((uint8_t*)data)[2] << 16;
            case 2: h2 ^= ((uint8_t*)data)[1] << 8;
            case 1: h2 ^= ((uint8_t*)data)[0];
                h2 *= m;
        };

        h1 ^= h2 >> 18; h1 *= m;
        h2 ^= h1 >> 22; h2 *= m;
        h1 ^= h2 >> 17; h1 *= m;
        h2 ^= h1 >> 19; h2 *= m;

        uint64_t h = h1;

        h = (h << 32) | h2;

        return h;
    }

    uint64_t FNV1AHash(const void* data, size_t count)
    {
        auto chardata = static_cast<const uint8_t*>(data);
        uint64_t value = 14695981039346656037ULL;

        for (size_t i = 0; i < count; ++i)
        {
            value ^= static_cast<size_t>(chardata[i]);
            value *= 1099511628211ULL;
        }

        return value;
    }

    // Note morton order performed poorly in comparison due to too regular offsets.
    // This works ok in cases where both a & b are linear low order values.
    uint64_t InterlaceHash32x2(uint32_t a, uint32_t b)
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
}