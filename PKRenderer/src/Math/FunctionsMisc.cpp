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

    void ReinterpretIndex16ToIndex32(uint32_t* dst, uint16_t* src, uint32_t count)
    {
        for (auto i = 0u; i < count; ++i)
        {
            dst[i] = (uint32_t)src[i];
        }
    }
}