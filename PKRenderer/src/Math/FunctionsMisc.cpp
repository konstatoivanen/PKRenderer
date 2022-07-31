#include "PrecompiledHeader.h"
#include "FunctionsMisc.h"
#include <iomanip>

namespace PK::Math::Functions
{
    void GetCascadeDepths(float znear, float zfar, float linearity, float* cascades, uint32_t count)
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

    uint32_t RandomUint()
    {
        auto v = rand();
        return *reinterpret_cast<uint32_t*>(&v);
    }
    
    float3 RandomFloat3() 
    { 
        return float3(RandomFloat(), RandomFloat(), RandomFloat()); 
    }

    uint32_t RandomRangeUint(uint32_t min, uint32_t max)
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

    float3 ToFloat3(float* ptr)
    {
        float3 value;
        memcpy(glm::value_ptr(value), ptr, sizeof(float) * 3);
        return value;
    }

    ushort PackHalf(float v)
    {
        if (v < -65536.0f)
        {
            v = -65536.0f;
        }

        if (v > 65536.0f)
        {
            v = 65536.0f;
        }

        v *= 1.925930e-34f;
        int32_t i = *(int32_t*)&v;
        uint32_t ui = (uint32_t)i;
        return ((i >> 16) & (int)0xffff8000) | ((int)(ui >> 13));
    }

    ushort2 PackHalf(float2 v)
    {
        return { PackHalf(v.x), PackHalf(v.y) };
    }

    ushort3 PackHalf(float3 v)
    {
        return { PackHalf(v.x), PackHalf(v.y), PackHalf(v.z) };
    }

    ushort4 PackHalf(float4 v)
    {
        return { PackHalf(v.x), PackHalf(v.y), PackHalf(v.z), PackHalf(v.w) };
    }

    float UnPackHalf(ushort v)
    {
        int32_t iv = v;
        int32_t i = (iv & 0x47fff) << 13;
        return *(float*)&i * 5.192297e+33f;
    }

    float2 UnPackHalf(ushort2 v)
    {
        return { UnPackHalf(v.x), UnPackHalf(v.y) };
    }

    float3 UnPackHalf(ushort3 v)
    {
        return { UnPackHalf(v.x), UnPackHalf(v.y), UnPackHalf(v.z) };
    }

    float4 UnPackHalf(ushort4 v)
    {
        return { UnPackHalf(v.x), UnPackHalf(v.y), UnPackHalf(v.z), UnPackHalf(v.w) };
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

    uint32_t GetMaxMipLevelPow2(uint32_t resolution) 
    { 
        return glm::log2(resolution); 
    }

    uint32_t GetMaxMipLevelPow2(uint2 resolution) 
    { 
        return glm::log2(glm::compMin(resolution)); 
    }

    uint32_t GetMaxMipLevelPow2(uint3 resolution) 
    { 
        return glm::log2(glm::compMin(resolution)); 
    }

    uint32_t GetMaxMipLevel(uint32_t resolution)
    {
        uint32_t level = 1;

        while (resolution > 1)
        {
            ++level;
            resolution >>= 1;
        }

        return level;
    }

    uint32_t GetMaxMipLevel(uint2 resolution) 
    { 
        return GetMaxMipLevel(glm::compMin(resolution)); 
    }

    uint32_t GetMaxMipLevel(uint3 resolution) 
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