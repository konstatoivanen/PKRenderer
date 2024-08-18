#include "PrecompiledHeader.h"
#include "Hash.h"

namespace PK::Hash
{
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
        const char* bytes = reinterpret_cast<const char*>(data);

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
            case 3: h2 ^= ((unsigned char*)data)[2] << 16;
            case 2: h2 ^= ((unsigned char*)data)[1] << 8;
            case 1: h2 ^= ((unsigned char*)data)[0];
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
        auto chardata = reinterpret_cast<const unsigned char*>(data);
        uint64_t value = 14695981039346656037ULL;

        for (size_t i = 0; i < count; ++i)
        {
            value ^= static_cast<size_t>(chardata[i]);
            value *= 1099511628211ULL;
        }

        return value;
    }
}