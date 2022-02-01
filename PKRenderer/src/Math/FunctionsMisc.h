#pragma once
#include "Types.h"

namespace PK::Math::Functions
{
    void GetCascadeDepths(float znear, float zfar, float linearity, float* cascades, uint32_t count);
    float CascadeDepth(float znear, float zfar, float linearity, float interpolant);
    float Cot(float value);
    float RandomFloat();
    uint32_t RandomUint();
    float3 RandomFloat3();
    uint32_t RandomRangeUint(uint32_t min, uint32_t max);
    float RandomRangeFloat(float min, float max);
    float3 RandomRangeFloat3(const float3& min, const float3& max);
    float3 RandomEuler();
    float3 ToFloat3(float* ptr);
    size_t GetNextExponentialSize(size_t start, size_t min);
    uint32_t GetMaxMipLevelPow2(uint32_t resolution);
    uint32_t GetMaxMipLevelPow2(uint2 resolution);
    uint32_t GetMaxMipLevelPow2(uint3 resolution);
    uint32_t GetMaxMipLevel(uint32_t resolution);
    uint32_t GetMaxMipLevel(uint2 resolution);
    uint32_t GetMaxMipLevel(uint3 resolution); 
    std::string BytesToString(size_t bytes, uint32_t decimalPlaces = 2);
    void ReinterpretIndex16ToIndex32(uint32_t* dst, uint16_t* src, uint32_t count);
}