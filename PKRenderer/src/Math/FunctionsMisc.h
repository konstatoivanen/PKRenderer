#pragma once
#include "Types.h"

namespace PK::Math::Functions
{
    void GetCascadeDepths(float znear, float zfar, float linearity, float* cascades, uint count);
    float CascadeDepth(float znear, float zfar, float linearity, float interpolant);
    float Cot(float value);
    float RandomFloat();
    float3 RandomFloat3();
    float RandomRangeFloat(float min, float max);
    float3 RandomRangeFloat3(const float3& min, const float3& max);
    float3 RandomEuler();
    size_t GetNextExponentialSize(size_t start, size_t min);
    uint GetMaxMipLevelPow2(uint resolution);
    uint GetMaxMipLevelPow2(uint2 resolution);
    uint GetMaxMipLevelPow2(uint3 resolution);
    uint GetMaxMipLevel(uint resolution);
    uint GetMaxMipLevel(uint2 resolution);
    uint GetMaxMipLevel(uint3 resolution); 
    uint ByteArrayHash(const void* data, size_t count);
    ulong MurmurHash(const void* data, size_t count, ulong seed);
    std::string BytesToString(size_t bytes, uint32_t decimalPlaces = 2);
}