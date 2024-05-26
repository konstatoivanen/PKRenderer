#include "PrecompiledHeader.h"
#include "FunctionsMisc.h"
#include <iomanip>

namespace PK::Math
{
    void GetCascadeDepths(float znear, float zfar, float linearity, float* cascades, uint32_t count)
    {
        assert(count > 2);

        count--;
        cascades[0] = znear;
        cascades[count] = zfar;

        for (auto i = 1u; i < count; ++i)
        {
            cascades[i] = CascadeDepth(znear, zfar, linearity, i / (float)count);
        }
    }

    void GetCascadeDepths(float znear, float zfar, float linearity, float* cascades, uint32_t gridSizeZ, uint32_t count)
    {
        assert(count > 2);

        GetCascadeDepths(znear, zfar, linearity, cascades, count);

        // Snap z ranges to tile indices to make shader branching more coherent
        auto scale = gridSizeZ / glm::log2(zfar / znear);
        auto bias = gridSizeZ * -log2(znear) / log2(zfar / znear);

        for (auto i = 1; i < (int32_t)(count - 1u); ++i)
        {
            float zTile = round(log2(cascades[i]) * scale + bias);
            cascades[i] = znear * pow(zfar / znear, zTile / gridSizeZ);
        }
    }

    float4 GetCascadeDepthsFloat4(float znear, float zfar, float linearity, uint32_t gridSizeZ)
    {
        float cascades[5]{};
        Math::GetCascadeDepths(znear, zfar, linearity, cascades, gridSizeZ, 5);
        return *reinterpret_cast<float4*>(cascades);
    }

    float CascadeDepth(float znear, float zfar, float linearity, float interpolant)
    {
        return linearity * (znear * powf(zfar / znear, interpolant)) + (1.0f - linearity) * (znear + (zfar - znear) * interpolant);
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
        return *reinterpret_cast<uint32_t*>(&v);
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

    ushort2 PackHalf(float2 v)
    {
        return { PackHalf(v.x), PackHalf(v.y) };
    }

    ushort3 PackHalf(float3 v)
    {
        return { PackHalf(v.x), PackHalf(v.y), PackHalf(v.z) };
    }

    ushort4 PackHalf(float4 v)
    {
        return { PackHalf(v.x), PackHalf(v.y), PackHalf(v.z), PackHalf(v.w) };
    }

    float UnPackHalf(ushort v)
    {
        int32_t iv = v;
        int32_t i = (iv & 0x47fff) << 13;
        return *(float*)&i * 5.192297e+33f;
    }

    float2 UnPackHalf(ushort2 v)
    {
        return { UnPackHalf(v.x), UnPackHalf(v.y) };
    }

    float3 UnPackHalf(ushort3 v)
    {
        return { UnPackHalf(v.x), UnPackHalf(v.y), UnPackHalf(v.z) };
    }

    float4 UnPackHalf(ushort4 v)
    {
        return { UnPackHalf(v.x), UnPackHalf(v.y), UnPackHalf(v.z), UnPackHalf(v.w) };
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

    uint32_t GetMaxMipLevelPow2(uint2 resolution)
    {
        return glm::log2(glm::compMin(resolution));
    }

    uint32_t GetMaxMipLevelPow2(uint3 resolution)
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

    uint32_t GetMaxMipLevel(uint2 resolution)
    {
        return GetMaxMipLevel(glm::compMin(resolution));
    }

    uint32_t GetMaxMipLevel(uint3 resolution)
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
        assert(alignment && ((alignment & (alignment - 1)) == 0));
        return (value + alignment - 1u) & ~(alignment - 1u);
    }

    uint64_t GetAlignedSize(uint64_t value, uint64_t alignment)
    {
        assert(alignment && ((alignment & (alignment - 1)) == 0));
        return (value + alignment - 1ull) & ~(alignment - 1ull);
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
        uint32_t count = 0;

        while (value > 0)
        {
            count += value & 1;
            value >>= 1;
        }

        return count;
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

    float2 OctaEncode(float3 n)
    {
        n /= (abs(n.x) + abs(n.y) + abs(n.z));
        n.xz = n.y >= 0.0f ? n.xz : OctaWrap(n.xz);
        n.xz = n.xz * 0.5f + 0.5f;
        return n.xz;
    }

    uint OctaEncodeUint(const float3& direction)
    {
        auto uv = OctaEncode(direction);
        auto x = (uint)glm::round(glm::clamp(uv.x, 0.0f, 1.0f) * 65535.0f);
        auto y = (uint)glm::round(glm::clamp(uv.y, 0.0f, 1.0f) * 65535.0f);
        return (x & 0xFFFFu) | ((y & 0xFFFFu) << 16u);
    }
}