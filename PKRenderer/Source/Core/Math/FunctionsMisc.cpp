#include "PrecompiledHeader.h"
#include <cassert>
#include "Core/Utilities/Memory.h"
#include "FunctionsMisc.h"

namespace PK::Math
{
    void GetCascadeDepths(float znear, float zfar, float distribution, float* cascades, uint32_t count)
    {
        assert(count > 2);

        count--;
        cascades[0] = znear;
        cascades[count] = zfar;

        const auto zparams = GetExponentialZParams01(znear, zfar, distribution);

        for (auto i = 1u; i < count; ++i)
        {
            cascades[i] = ClipToViewDepthExp(i / (float)count, zparams);
        }
    }

    void GetCascadeDepths(float znear, float zfar, float distribution, float* cascades, const float3& zAlignParams, uint32_t count)
    {
        assert(count > 2);

        GetCascadeDepths(znear, zfar, distribution, cascades, count);

        // Snap z ranges to tile indices to make shader branching more coherent
        for (auto i = 1; i < (int32_t)(count - 1u); ++i)
        {
            float zcoord = math::round(ViewToClipDepthExp(cascades[i], zAlignParams));
            cascades[i] = ClipToViewDepthExp(zcoord, zAlignParams);
        }
    }

    float4 GetCascadeDepthsFloat4(float znear, float zfar, float distribution, const float3& zAlignParams)
    {
        float cascades[5]{};
        GetCascadeDepths(znear, zfar, distribution, cascades, zAlignParams, 5);
        // Ignore first plane as it is just the near plane and we are more interested in the final clipping plane.
        return Memory::BitCast<float, float4>(cascades + 1u);
    }

    float3 GetExponentialZParams01(float znear, float zfar, float distribution)
    {
        const auto o = (zfar - znear * math::exp2(1.0f / distribution)) / (zfar - znear);
        const auto b = (1.0f - o) / znear;
        return float3(b,o, distribution);
    }

    float3 GetExponentialZParams(float znear, float zfar, float distribution, uint32_t size)
    {
        const auto o = (zfar - znear * math::exp2((size - 1.0f) / distribution)) / (zfar - znear);
        const auto b = (1.0f - o) / znear;
        return float3(b, o, distribution);
    }

    float ViewToClipDepthExp(float viewz, const float3& params)
    {
        return math::log2(viewz * params.x + params.y) * params.z;
    }

    float ClipToViewDepthExp(float clipz, const float3& params)
    {
        return (math::exp2(clipz / params.z) - params.y) / params.x;
    }

    float2 OctaWrap(const float2& v) { return (1.0f - math::abs(float2(v.yx))) * float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0); }

    float2 OctaEncode(const float3& n)
    {
        auto v = n;
        v /= (math::abs(v.x) + math::abs(v.y) + math::abs(v.z));
        v.xz = v.y >= 0.0f ? v.xz : OctaWrap(v.xz);
        v.xz = v.xz * 0.5f + 0.5f;
        return v.xz;
    }

    uint OctaEncodeUint(const float3& direction)
    {
        auto uv = OctaEncode(direction);
        auto x = (uint)math::round(math::clamp(uv.x, 0.0f, 1.0f) * 65535.0f);
        auto y = (uint)math::round(math::clamp(uv.y, 0.0f, 1.0f) * 65535.0f);
        return (x & 0xFFFFu) | ((y & 0xFFFFu) << 16u);
    }

    float3 GetTriangleNormal(const float* p0, const float* p1, const float* p2, bool& outIsValid)
    {
        const float p10[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
        const float p20[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
        const float normalx = p10[1] * p20[2] - p10[2] * p20[1];
        const float normaly = p10[2] * p20[0] - p10[0] * p20[2];
        const float normalz = p10[0] * p20[1] - p10[1] * p20[0];
        const float area = sqrtf(normalx * normalx + normaly * normaly + normalz * normalz);
        outIsValid = area != 0.0f;
        return float3(normalx, normaly, normalz) / area;
    }

    float3 GetTriangleNormal(const float3& a, const float3& b, const float3& c)
    {
        bool isValid = false;
        return GetTriangleNormal(&a.x, &b.x, &c.x, isValid);
    }
}
