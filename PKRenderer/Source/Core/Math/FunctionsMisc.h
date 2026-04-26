#pragma once
#include "Forward.h"

namespace PK::Math
{
    void GetCascadeDepths(float znear, float zfar, float distribution, float* cascades, uint32_t count);
    void GetCascadeDepths(float znear, float zfar, float distribution, float* cascades, const float3& zAlignParams, uint32_t count);
    float4 GetCascadeDepthsFloat4(float znear, float zfar, float distribution, const float3& zAlignParams);
    float3 GetExponentialZParams01(float znear, float zfar, float distribution);
    float3 GetExponentialZParams(float znear, float zfar, float distribution, uint32_t size);
    float ViewToClipDepthExp(float viewz, const float3& params);
    float ClipToViewDepthExp(float viewz, const float3& params);
    float RandomFloat();
    float GetHaltonSequence(uint32_t index, uint32_t radix);
    uint32_t RandomUint();
    float3 RandomFloat3();
    uint32_t RandomRangeUint(uint32_t min, uint32_t max);
    float RandomRangeFloat(float min, float max);
    float3 RandomRangeFloat3(const float3& min, const float3& max);
    float3 RandomEuler();
    float3 SafeNormalize(const float3& v);
    uint32_t GetMaxMipLevel(uint32_t resolution);
    uint32_t GetMaxMipLevel(const uint2& resolution);
    uint32_t GetMaxMipLevel(const uint3& resolution);
    void ReinterpretIndex16ToIndex32(uint32_t* dst, uint16_t* src, uint32_t count);
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
}
