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
    float2 OctaWrap(const float2& v);
    float2 OctaEncode(const float3& n);
    float3 GetTriangleNormal(const float* p0, const float* p1, const float* p2, bool& outIsValid);
    float3 GetTriangleNormal(const float3& a, const float3& b, const float3& c);
}
