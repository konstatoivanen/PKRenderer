#pragma once
#include "MathFwd.h"

namespace PK::Math
{
    void GetCascadeDepths(float znear, float zfar, float linearity, float* cascades, uint32_t count);
    void GetCascadeDepths(float znear, float zfar, float linearity, float* cascades, uint32_t gridSizeZ, uint32_t count);
    float4 GetCascadeDepthsFloat4(float znear, float zfar, float linearity, uint32_t gridSizeZ);
    float CascadeDepth(float znear, float zfar, float linearity, float interpolant);
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
    ushort2 PackHalf(float2 v);
    ushort3 PackHalf(float3 v);
    ushort4 PackHalf(float4 v);
    float UnPackHalf(ushort v);
    float2 UnPackHalf(ushort2 v);
    float3 UnPackHalf(ushort3 v);
    float4 UnPackHalf(ushort4 v);
    size_t GetNextExponentialSize(size_t start, size_t min);
    uint32_t GetMaxMipLevelPow2(uint32_t resolution);
    uint32_t GetMaxMipLevelPow2(uint2 resolution);
    uint32_t GetMaxMipLevelPow2(uint3 resolution);
    uint32_t GetMaxMipLevel(uint32_t resolution);
    uint32_t GetMaxMipLevel(uint2 resolution);
    uint32_t GetMaxMipLevel(uint3 resolution);
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
    float2 OctaEncode(float3 n);
    uint OctaEncodeUint(const float3& direction);
}
