#include "PrecompiledHeader.h"
#include "Random.h"

namespace PK::math
{
    thread_local pcg32_state s_thread_pcg_state;

    void setSeed(pcg32_state* rng, uint64_t state, uint64_t sequence)
    {
        rng->state = 0U;
        rng->inc = (sequence << 1u) | 1u;
        randomUint(rng);
        rng->state += state;
        randomUint(rng);
    }

    void setSeed(uint64_t state, uint64_t sequence)
    {
        srand(static_cast<uint32_t>(sequence));
        setSeed(&s_thread_pcg_state, state, sequence);
    }

    uint32_t randomUint(pcg32_state* rng)
    {
        uint64_t oldstate = rng->state;
        rng->state = oldstate * 6364136223846793005ULL + rng->inc;
        uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
        uint32_t rot0 = oldstate >> 59u;
        uint32_t rot1 = static_cast<uint32_t>(-static_cast<int32_t>(rot0) & 31);
        return (xorshifted >> rot0) | (xorshifted << rot1);
    }

    uint32_t randomUint()
    { 
        return randomUint(&s_thread_pcg_state);
    }

    float halton(uint32_t index, uint32_t radix)
    {
        float result = 0.0f;
        float fraction = 1.0f / (float)radix;

        while (index > 0u)
        {
            result += (float)(index % radix) * fraction;

            index /= radix;
            fraction /= (float)radix;
        }

        return result;
    }

    vector<float, 2> hammersley(uint32_t i, uint32_t n)
    {
        uint32_t bits = i;
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        float y = float(bits) * 2.3283064365386963e-10f;
        return vector<float,2>(float(i % n) / float(n), y);
    }

    uint32_t murmurhash11(uint32_t seed)
    {
        const uint32_t M = 0x5bd1e995u;
        uint32_t h = 1190494759u;
        seed *= M; seed ^= seed >> 24u; seed *= M;
        h *= M; h ^= seed;
        h ^= h >> 13u; h *= M; h ^= h >> 15u;
        return h;
    }

    vector<uint32_t, 2> murmurhash21(uint32_t seed)
    {
        const uint M = 0x5bd1e995u;
        uint2 h = uint2(1190494759u, 2147483647u);
        seed *= M; seed ^= seed >> 24u; seed *= M;
        h *= M; h ^= seed;
        h ^= h >> 13u; h *= M; h ^= h >> 15u;
        return h;
    }

    vector<uint32_t, 3> murmurhash31(uint32_t seed)
    {
        const uint M = 0x5bd1e995u;
        uint3 h = uint3(1190494759u, 2147483647u, 3559788179u);
        seed *= M; seed ^= seed >> 24u; seed *= M;
        h *= M; h ^= seed;
        h ^= h >> 13u; h *= M; h ^= h >> 15u;
        return h;
    }

    vector<uint32_t, 4> murmurhash41(uint32_t seed)
    {
        const uint M = 0x5bd1e995u;
        uint4 h = uint4(1190494759u, 2147483647u, 3559788179u, 179424673u);
        seed *= M; seed ^= seed >> 24u; seed *= M;
        h *= M; h ^= seed;
        h ^= h >> 13u; h *= M; h ^= h >> 15u;
        return h;
    }
}
