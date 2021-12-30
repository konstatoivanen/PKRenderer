#include "PrecompiledHeader.h"
#include "HashHelpers.h"

namespace PK::Utilities::HashHelpers
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

    int32_t ExpandPrime(uint32_t oldSize)
    {
        int32_t newSize = 2 * oldSize;

        // Allow the hashtables to grow to maximum possible size (~2G elements) before encoutering capacity overflow.
        // Note that this check works even when _items.Length overflowed thanks to the (uint) cast
        if ((uint32_t)newSize > MaxPrimeArrayLength && MaxPrimeArrayLength > oldSize)
        {
            if (MaxPrimeArrayLength != GetPrime(MaxPrimeArrayLength))
            {
                // Invalid MaxPrimeArrayLength;
                return -1;
            }

            return MaxPrimeArrayLength;
        }

        return GetPrime(newSize);
    }
}