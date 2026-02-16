#include "PrecompiledHeader.h"
#include "FunctionsMisc.h"
#include <iomanip>

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
            float zcoord = round(ViewToClipDepthExp(cascades[i], zAlignParams));
            cascades[i] = ClipToViewDepthExp(zcoord, zAlignParams);
        }
    }

    float4 GetCascadeDepthsFloat4(float znear, float zfar, float distribution, const float3& zAlignParams)
    {
        float cascades[5]{};
        GetCascadeDepths(znear, zfar, distribution, cascades, zAlignParams, 5);
        // Ignore first plane as it is just the near plane and we are more interested in the final clipping plane.
        return BitCast<float, float4>(cascades + 1u);
    }

    float3 GetExponentialZParams01(float znear, float zfar, float distribution)
    {
        const auto o = (zfar - znear * glm::exp2(1.0f / distribution)) / (zfar - znear);
        const auto b = (1.0f - o) / znear;
        return float3(b,o, distribution);
    }

    float3 GetExponentialZParams(float znear, float zfar, float distribution, uint32_t size)
    {
        const auto o = (zfar - znear * glm::exp2((size - 1.0f) / distribution)) / (zfar - znear);
        const auto b = (1.0f - o) / znear;
        return float3(b, o, distribution);
    }

    float ViewToClipDepthExp(float viewz, const float3& params)
    {
        return log2(viewz * params.x + params.y) * params.z;
    }

    float ClipToViewDepthExp(float clipz, const float3& params)
    {
        return (exp2(clipz / params.z) - params.y) / params.x;
    }

    float Cot(float value)
    {
        return cos(value) / sin(value);
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
        return BitCast<int32_t, uint32_t>(v);
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

    ushort2 PackHalf(const float2& v)
    {
        return { PackHalf(v.x), PackHalf(v.y) };
    }

    ushort3 PackHalf(const float3& v)
    {
        return { PackHalf(v.x), PackHalf(v.y), PackHalf(v.z) };
    }

    ushort4 PackHalf(const float4& v)
    {
        return { PackHalf(v.x), PackHalf(v.y), PackHalf(v.z), PackHalf(v.w) };
    }

    uint PackHalfToUint(const float2& v)
    {
        auto p = PackHalf(v);
        return (p.x & 0xFFFFu) | (p.y << 16u);
    }

    float UnPackHalf(ushort v)
    {
        int32_t iv = v;
        int32_t i = (iv & 0x47fff) << 13;
        return *(float*)&i * 5.192297e+33f;
    }

    float2 UnPackHalf(const ushort2& v)
    {
        return { UnPackHalf(v.x), UnPackHalf(v.y) };
    }

    float3 UnPackHalf(const ushort3& v)
    {
        return { UnPackHalf(v.x), UnPackHalf(v.y), UnPackHalf(v.z) };
    }

    float4 UnPackHalf(const ushort4& v)
    {
        return { UnPackHalf(v.x), UnPackHalf(v.y), UnPackHalf(v.z), UnPackHalf(v.w) };
    }

    uint FloatAsUint(float v)
    {
        return BitCast<float, uint>(v);
    }

    uint2 FloatAsUint(const float2& v)
    {
        return BitCast<uint2, float2>(v);
    }

    uint3 FloatAsUint(const float3& v)
    {
        return BitCast<float3, uint3>(v);
    }

    uint4 FloatAsUint(const float4& v)
    {
        return BitCast<float4, uint4>(v);
    }

    float3 SafeNormalize(const float3& v)
    {
        float length = glm::length(v);
        return v * (length == 0.0f ? 0.0f : (1.0f / length));
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

    uint32_t GetMaxMipLevelPow2(const uint2& resolution)
    {
        return glm::log2(glm::compMin(resolution));
    }

    uint32_t GetMaxMipLevelPow2(const uint3& resolution)
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

    uint32_t GetMaxMipLevel(const uint2& resolution)
    {
        return GetMaxMipLevel(glm::compMin(resolution));
    }

    uint32_t GetMaxMipLevel(const uint3& resolution)
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

    uint32_t GetAlignedSize(uint32_t value, uint32_t alignment)
    {
        const auto remainder = value % alignment;
        return value + (remainder ? (alignment - remainder) : 0u);
    }

    uint64_t GetAlignedSize(uint64_t value, uint64_t alignment)
    {
        const auto remainder = value % alignment;
        return value + (remainder ? (alignment - remainder) : 0ull);
    }

    uint2 GetAlignedSize(const uint2& resolution, uint32_t alignment)
    {
        return
        {
            GetAlignedSize(resolution.x, alignment),
            GetAlignedSize(resolution.y, alignment),
        };
    }

    uint3 GetAlignedSize(const uint3& resolution, uint32_t alignment)
    {
        return
        {
            GetAlignedSize(resolution.x, alignment),
            GetAlignedSize(resolution.y, alignment),
            GetAlignedSize(resolution.z, alignment)
        };
    }

    uint3 GetAlignedSizeXY(const uint3& resolution, uint32_t alignment)
    {
        return
        {
            GetAlignedSize(resolution.x, alignment),
            GetAlignedSize(resolution.y, alignment),
            resolution.z
        };
    }

    uint4 GetAlignedSize(const uint4& value, uint32_t alignment)
    {
        return
        {
            GetAlignedSize(value.x, alignment),
            GetAlignedSize(value.y, alignment),
            GetAlignedSize(value.z, alignment),
            GetAlignedSize(value.w, alignment)
        };
    }

    uint3 GetComputeGroupCount(const uint3& threads, const uint3& clusterSize)
    {
        return
        {
            (uint)glm::ceil(threads.x / float(clusterSize.x)),
            (uint)glm::ceil(threads.y / float(clusterSize.y)),
            (uint)glm::ceil(threads.z / float(clusterSize.z))
        };
    }

    uint32_t CountBits(uint32_t value)
    {
        value = value - ((value >> 1) & 0x55555555);
        value = (value & 0x33333333) + ((value >> 2) & 0x33333333);
        value = (value + (value >> 4)) & 0x0F0F0F0F;
        value *= 0x01010101;
        return value >> 24;
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

    float2 OctaWrap(const float2& v) { return (1.0f - glm::abs(float2(v.yx))) * float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0); }

    float2 OctaEncode(const float3& n)
    {
        auto v = n;
        v /= (abs(v.x) + abs(v.y) + abs(v.z));
        v.xz = v.y >= 0.0f ? v.xz : OctaWrap(v.xz);
        v.xz = v.xz * 0.5f + 0.5f;
        return v.xz;
    }

    uint OctaEncodeUint(const float3& direction)
    {
        auto uv = OctaEncode(direction);
        auto x = (uint)glm::round(glm::clamp(uv.x, 0.0f, 1.0f) * 65535.0f);
        auto y = (uint)glm::round(glm::clamp(uv.y, 0.0f, 1.0f) * 65535.0f);
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
        return GetTriangleNormal(glm::value_ptr(a), glm::value_ptr(b), glm::value_ptr(c), isValid);
    }
}
