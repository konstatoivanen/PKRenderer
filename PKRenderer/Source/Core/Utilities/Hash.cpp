#include "PrecompiledHeader.h"
#include "Hash.h"

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
        1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369 };

    bool IsPrime(int32_t candidate)
    {
        if ((candidate & 1) != 0)
        {
            auto limit = (int32_t)sqrt(candidate);

            for (int32_t divisor = 3; divisor <= limit; divisor += 2)
            {
                if ((candidate % divisor) == 0)
                {
                    return false;
                }
            }

            return true;
        }

        return (candidate == 2);
    }

    int32_t GetPrime(int32_t min)
    {
        if (min < 0)
        {
            // Invalid argument
            return -1;
        }

        if (min == 0)
        {
            return 0;
        }

        for (int32_t i = 0; i < PrimesLength; ++i)
        {
            int32_t prime = Primes[i];

            if (prime >= min)
            {
                return prime;
            }
        }

        //outside of our predefined table. 
        //compute the hard way. 
        const auto maxvalue = 2147483647;

        for (int32_t i = (min | 1); i < maxvalue; i += 2)
        {
            if (IsPrime(i) && ((i - 1) % HashPrime != 0))
            {
                return i;
            }
        }

        return min;
    }

    uint32_t ExpandPrime(uint32_t oldSize)
    {
        int32_t newSize = 2 * oldSize;

        // Allow the hashtables to grow to maximum possible size (~2G elements) before encoutering capacity overflow.
        // Note that this check works even when _items.Length overflowed thanks to the (uint) cast
        if ((uint32_t)newSize > MaxPrimeArrayLength && MaxPrimeArrayLength > oldSize)
        {
            if (MaxPrimeArrayLength != GetPrime(MaxPrimeArrayLength))
            {
                // Invalid MaxPrimeArrayLength;
                return oldSize * 2;
            }

            return MaxPrimeArrayLength;
        }

        return (uint32_t)GetPrime(newSize);
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