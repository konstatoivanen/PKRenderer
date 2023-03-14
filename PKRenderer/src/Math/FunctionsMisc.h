#pragma once
#include "Types.h"

namespace PK::Math::Functions
{
    void GetCascadeDepths(float znear, float zfar, float linearity, float* cascades, uint32_t count);
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
    uint3 GetComputeGroupCount(const uint3& threads, const uint3& clusterSize);
    uint32_t CountBits(uint32_t value);
    inline uint64_t ULongAdd(uint64_t a, int32_t b) { return (int64_t)a + b < 0 ? 0ull : a + b; }
}