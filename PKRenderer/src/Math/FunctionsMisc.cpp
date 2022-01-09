#include "PrecompiledHeader.h"
#include "FunctionsMisc.h"
#include <iomanip>

namespace PK::Math::Functions
{
    void GetCascadeDepths(float znear, float zfar, float linearity, float* cascades, uint count)
    {
        assert(count > 2);

        count--;
        cascades[0] = znear;
        cascades[count] = zfar;

        for (auto i = 1u; i < count; ++i)
        {
            cascades[i] = CascadeDepth(znear, zfar, linearity, i / (float)count);
        }
    }
    
    float CascadeDepth(float znear, float zfar, float linearity, float interpolant) 
    { 
        return linearity * (znear * powf(zfar / znear, interpolant)) + (1.0f - linearity) * (znear + (zfar - znear) * interpolant); 
    }
    
    float Cot(float value) 
    { 
        return cos(value) / sin(value); 
    }
    
    float RandomFloat() 
    { 
        return (float)rand() / (float)RAND_MAX;
    }

    uint RandomUint()
    {
        auto v = rand();
        return *reinterpret_cast<uint*>(&v);
    }
    
    float3 RandomFloat3() 
    { 
        return float3(RandomFloat(), RandomFloat(), RandomFloat()); 
    }

    uint RandomRangeUint(uint min, uint max)
    {
        return  min + (RandomUint() % (max - min));
    }

    float RandomRangeFloat(float min, float max) 
    { 
        return min + (RandomFloat() * (max - min)); 
    }

    float3 RandomRangeFloat3(const float3& min, const float3& max) 
    { 
        return float3(RandomRangeFloat(min.x, max.x), RandomRangeFloat(min.y, max.y), RandomRangeFloat(min.z, max.z)); 
    }
   
    float3 RandomEuler() 
    { 
        return float3(RandomRangeFloat(-360.0f, 360.0f), RandomRangeFloat(-360.0f, 360.0f), RandomRangeFloat(-360.0f, 360.0f)); 
    }

    size_t GetNextExponentialSize(size_t start, size_t min)
    {
        if (start < 1)
        {
            start = 1;
        }

        while (start < min)
        {
            start <<= 1;
        }

        return start;
    }

    uint GetMaxMipLevelPow2(uint resolution) 
    { 
        return glm::log2(resolution); 
    }

    uint GetMaxMipLevelPow2(uint2 resolution) 
    { 
        return glm::log2(glm::compMin(resolution)); 
    }

    uint GetMaxMipLevelPow2(uint3 resolution) 
    { 
        return glm::log2(glm::compMin(resolution)); 
    }

    uint GetMaxMipLevel(uint resolution)
    {
        uint level = 1;

        while (resolution > 1)
        {
            ++level;
            resolution >>= 1;
        }

        return level;
    }

    uint GetMaxMipLevel(uint2 resolution) 
    { 
        return GetMaxMipLevel(glm::compMin(resolution)); 
    }

    uint GetMaxMipLevel(uint3 resolution) 
    { 
        return GetMaxMipLevel(glm::compMin(resolution)); 
    }

    uint ByteArrayHash(const void* data, size_t count)
    {
        const char* bytes = reinterpret_cast<const char*>(data);

        // Source: https://stackoverflow.com/questions/16340/how-do-i-generate-a-hashcode-from-a-byte-array-in-c
        const auto p = 16777619;
        auto hash = 2166136261;

        for (int i = 0; i < count; ++i)
        {
            hash = (hash ^ bytes[i]) * p;
        }

        hash += hash << 13;
        hash ^= hash >> 7;
        hash += hash << 3;
        hash ^= hash >> 17;
        hash += hash << 5;
        return (uint)hash;
    }

    ulong MurmurHash(const void* key, size_t len, ulong seed)
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

    std::string BytesToString(size_t bytes, uint32_t decimalPlaces)
    {
        if (bytes == 0)
        {
            return "0bytes";
        }

        auto mag = (int)(log(bytes) / log(1024));
        mag = glm::min(mag, 2);

        auto adjustedSize = (double)bytes / (1L << (mag * 10));
        auto factor = pow(10, decimalPlaces);

        if ((round(adjustedSize * factor) / factor) >= 1000)
        {
            mag += 1;
            adjustedSize /= 1024;
        }

        std::stringstream stream;
        stream << std::fixed << std::setprecision(decimalPlaces) << adjustedSize;
        stream << (mag > 1 ? "MB" : mag > 0 ? "KB" : "bytes");
        return stream.str();
    }
}