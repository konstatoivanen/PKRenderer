#pragma once
#include "MathFwd.h"

namespace PK::Math
{
    void GetCascadeDepths(float znear, float zfar, float distribution, float* cascades, uint32_t count);
    void GetCascadeDepths(float znear, float zfar, float distribution, float* cascades, const float3& zAlignParams, uint32_t count);
    float4 GetCascadeDepthsFloat4(float znear, float zfar, float distribution, const float3& zAlignParams);
    float3 GetExponentialZParams01(float znear, float zfar, float distribution);
    float3 GetExponentialZParams(float znear, float zfar, float distribution, uint32_t size);
    float ViewToClipDepthExp(float viewz, const float3& params);
    float ClipToViewDepthExp(float viewz, const float3& params);
    float Cot(float value);
    float RandomFloat();
    float GetHaltonSequence(uint32_t index, uint32_t radix);
    uint32_t RandomUint();
    float3 RandomFloat3();
    uint32_t RandomRangeUint(uint32_t min, uint32_t max);
    float RandomRangeFloat(float min, float max);
    float3 RandomRangeFloat3(const float3& min, const float3& max);
    float3 RandomEuler();
    float3 ToFloat3(float* ptr);
    ushort PackHalf(float v);
    ushort2 PackHalf(const float2& v);
    ushort3 PackHalf(const float3& v);
    ushort4 PackHalf(const float4& v);
    uint PackHalfToUint(const float2& v);
    float UnPackHalf(ushort v);
    float2 UnPackHalf(const ushort2& v);
    float3 UnPackHalf(const ushort3& v);
    float4 UnPackHalf(const ushort4& v);
    uint FloatAsUint(float v);
    uint2 FloatAsUint(const float2& v);
    uint3 FloatAsUint(const float3& v);
    uint4 FloatAsUint(const float4& v);
    float3 SafeNormalize(const float3& v);
    size_t GetNextExponentialSize(size_t start, size_t min);
    uint32_t GetMaxMipLevelPow2(uint32_t resolution);
    uint32_t GetMaxMipLevelPow2(const uint2& resolution);
    uint32_t GetMaxMipLevelPow2(const uint3& resolution);
    uint32_t GetMaxMipLevel(uint32_t resolution);
    uint32_t GetMaxMipLevel(const uint2& resolution);
    uint32_t GetMaxMipLevel(const uint3& resolution);
    std::string BytesToString(size_t bytes, uint32_t decimalPlaces = 2);
    void ReinterpretIndex16ToIndex32(uint32_t* dst, uint16_t* src, uint32_t count);
    uint32_t GetAlignedSize(uint32_t value, uint32_t alignment);
    uint64_t GetAlignedSize(uint64_t value, uint64_t alignment);
    uint2 GetAlignedSize(const uint2& resolution, uint32_t alignment);
    uint3 GetAlignedSize(const uint3& resolution, uint32_t alignment);
    uint3 GetAlignedSizeXY(const uint3& resolution, uint32_t alignment);
    uint4 GetAlignedSize(const uint4& value, uint32_t alignment);
    uint3 GetComputeGroupCount(const uint3& threads, const uint3& clusterSize);
    uint32_t CountBits(uint32_t value);
    inline uint64_t ULongAdd(uint64_t a, int32_t b) { return (int64_t)a + b < 0 ? 0ull : a + b; }
    uint4 MurmurHash41(uint32_t seed);
    uint2 MurmurHash21(uint32_t seed);
    float2 OctaWrap(const float2& v);
    float2 OctaEncode(const float3& n);
    uint OctaEncodeUint(const float3& direction);
    int32_t QuantizeSNorm(float v, int32_t n);
    sbyte3 QuantizeSNorm(const float3& v, int32_t n);
    float3 GetTriangleNormal(const float* p0, const float* p1, const float* p2, bool& outIsValid);
    float3 GetTriangleNormal(const float3& a, const float3& b, const float3& c);

    template<typename T0, typename T1>
    T1 BitCast(const T0& value)
    {
        static_assert(sizeof(T0) == sizeof(T1));
        T1 ret;
        memcpy(&ret, &value, sizeof(T0));
        return ret;
    }

    template<typename T0, typename T1>
    T1 BitCast(const T0* ptr)
    {
        T1 ret;
        memcpy(&ret, ptr, sizeof(T1));
        return ret;
    }
}
