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

    float RandomFloat()
    {
        return (float)rand() / (float)RAND_MAX;
    }

    float GetHaltonSequence(uint32_t index, uint32_t radix)
    {
        float result = 0.0f;
        float fraction = 1.0f / (float)radix;

        while (index > 0)
        {
            result += (float)(index % radix) * fraction;

            index /= radix;
            fraction /= (float)radix;
        }

        return result;
    }

    uint32_t RandomUint()
    {
        auto v = rand();
        return Memory::BitCast<int32_t, uint32_t>(v);
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

    float3 SafeNormalize(const float3& v)
    {
        float length = math::length(v);
        return v * (length == 0.0f ? 0.0f : (1.0f / length));
    }

    uint32_t GetMaxMipLevel(uint32_t resolution)
    {
        return resolution > 0u ? math::log2(resolution) + 1u : 0u;
    }

    uint32_t GetMaxMipLevel(const uint2& resolution)
    {
        return GetMaxMipLevel(math::cmin(resolution));
    }

    uint32_t GetMaxMipLevel(const uint3& resolution)
    {
        return GetMaxMipLevel(math::cmin(resolution));
    }

    void ReinterpretIndex16ToIndex32(uint32_t* dst, uint16_t* src, uint32_t count)
    {
        for (auto i = 0u; i < count; ++i)
        {
            dst[i] = (uint32_t)src[i];
        }
    }

    uint4 MurmurHash41(uint32_t seed)
    {
        const uint M = 0x5bd1e995u;
        uint4 h = uint4(1190494759u, 2147483647u, 3559788179u, 179424673u);
        seed *= M;
        seed ^= seed >> 24u;
        seed *= M;
        h *= M;
        h ^= seed;
        h ^= h >> 13u;
        h *= M;
        h ^= h >> 15u;
        return h;
    }

    uint2 MurmurHash21(uint32_t seed)
    {
        const uint M = 0x5bd1e995u;
        uint2 h = uint2(1190494759u, 2147483647u);
        seed *= M;
        seed ^= seed >> 24u;
        seed *= M;
        h *= M;
        h ^= seed;
        h ^= h >> 13u;
        h *= M;
        h ^= h >> 15u;
        return h;
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

    int32_t QuantizeSNorm(float v, int32_t n)
    {
        const float scale = float((1 << (n - 1)) - 1);
        float round = (v >= 0 ? 0.5f : -0.5f);
        v = (v >= -1) ? v : -1;
        v = (v <= +1) ? v : +1;
        return int32_t(v * scale + round);
    }

    sbyte3 QuantizeSNorm(const float3& v, int32_t n)
    {
        return
        {
            (sbyte)(QuantizeSNorm(v.x, n)),
            (sbyte)(QuantizeSNorm(v.y, n)),
            (sbyte)(QuantizeSNorm(v.z, n))
        };
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
