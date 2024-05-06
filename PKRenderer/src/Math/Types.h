#pragma once
#define GLM_FORCE_SWIZZLE 
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace PK::Math
{
    typedef uint16_t ushort;
    typedef uint32_t uint;
    typedef uint64_t ulong;
    typedef std::uint8_t byte;
    typedef std::int8_t sbyte;


    typedef glm::vec2 float2;
    typedef glm::vec3 float3;
    typedef glm::vec4 float4;

    typedef glm::dvec2 double2;
    typedef glm::dvec3 double3;
    typedef glm::dvec4 double4;

    typedef glm::mat2x2 float2x2;
    typedef glm::mat3x3 float3x3;
    typedef glm::mat4x4 float4x4;
    typedef glm::mat3x4 float3x4;

    typedef glm::dmat2x2 double2x2;
    typedef glm::dmat3x3 double3x3;
    typedef glm::dmat4x4 double4x4;

    typedef glm::i16vec2 short2;
    typedef glm::i16vec3 short3;
    typedef glm::i16vec4 short4;

    typedef glm::u16vec2 ushort2;
    typedef glm::u16vec3 ushort3;
    typedef glm::u16vec4 ushort4;

    typedef glm::u16mat2x2 ushort2x2;
    typedef glm::u16mat3x3 ushort3x3;
    typedef glm::u16mat4x4 ushort4x4;

    typedef glm::lowp_i8vec4 sbyte4;
    typedef glm::lowp_u8vec4 byte4;

    typedef glm::ivec2 int2;
    typedef glm::ivec3 int3;
    typedef glm::ivec4 int4;

    typedef glm::uvec2 uint2;
    typedef glm::uvec3 uint3;
    typedef glm::uvec4 uint4;

    typedef glm::i64vec2 long2;
    typedef glm::i64vec3 long3;
    typedef glm::i64vec4 long4;

    typedef glm::u64vec2 ulong2;
    typedef glm::u64vec3 ulong3;
    typedef glm::u64vec4 ulong4;

    typedef glm::bvec2 bool2;
    typedef glm::bvec3 bool3;
    typedef glm::bvec4 bool4;

    typedef glm::lowp_u8vec4 color32;
    typedef glm::vec4 color;

    typedef glm::quat quaternion;

    constexpr float2 PK_FLOAT2_ZERO = { 0.0f, 0.0f };
    constexpr float3 PK_FLOAT3_ZERO = { 0.0f, 0.0f, 0.0f };
    constexpr float4 PK_FLOAT4_ZERO = { 0.0f, 0.0f, 0.0f, 0.0f };

    constexpr int2 PK_INT2_ZERO = { 0, 0 };
    constexpr int3 PK_INT3_ZERO = { 0, 0, 0 };
    constexpr int4 PK_INT4_ZERO = { 0, 0, 0, 0 };

    constexpr uint2 PK_UINT2_ZERO = { 0, 0 };
    constexpr uint3 PK_UINT3_ZERO = { 0, 0, 0 };
    constexpr uint4 PK_UINT4_ZERO = { 0, 0, 0, 0 };
    constexpr uint4 PK_UINT4_MAX = { 0, 0, ~0u, ~0u };

    constexpr bool2 PK_BOOL2_ZERO = { false, false };
    constexpr bool3 PK_BOOL3_ZERO = { false, false, false };
    constexpr bool4 PK_BOOL4_ZERO = { false, false, false, false };

    constexpr float2 PK_FLOAT2_ONE = { 1.0f, 1.0f };
    constexpr float3 PK_FLOAT3_ONE = { 1.0f, 1.0f, 1.0f };
    constexpr float4 PK_FLOAT4_ONE = { 1.0f, 1.0f, 1.0f, 1.0f };

    constexpr int2 PK_INT2_ONE = { 1, 1 };
    constexpr int3 PK_INT3_ONE = { 1, 1, 1 };
    constexpr int4 PK_INT4_ONE = { 1, 1, 1, 1 };

    constexpr uint2 PK_UINT2_ONE = { 1, 1 };
    constexpr uint3 PK_UINT3_ONE = { 1, 1, 1 };
    constexpr uint4 PK_UINT4_ONE = { 1, 1, 1, 1 };

    constexpr bool2 PK_BOOL2_ONE = { true, true };
    constexpr bool3 PK_BOOL3_ONE = { true, true, true };
    constexpr bool4 PK_BOOL4_ONE = { true, true, true, true };

    constexpr float2 PK_FLOAT2_UP = { 0.0f, 1.0f };
    constexpr float2 PK_FLOAT2_DOWN = { 0.0f, -1.0f };
    constexpr float2 PK_FLOAT2_LEFT = { -1.0f, 0.0f };
    constexpr float2 PK_FLOAT2_RIGHT = { 1.0f, 0.0f };

    constexpr float3 PK_FLOAT3_LEFT = { 1.0f,  0.0f,  0.0f };
    constexpr float3 PK_FLOAT3_RIGHT = { -1.0f,  0.0f,  0.0f };
    constexpr float3 PK_FLOAT3_UP = { 0.0f,  1.0f,  0.0f };
    constexpr float3 PK_FLOAT3_DOWN = { 0.0f, -1.0f,  0.0f };
    constexpr float3 PK_FLOAT3_FORWARD = { 0.0f,  0.0f,  1.0f };
    constexpr float3 PK_FLOAT3_BACKWARD = { 0.0f,  0.0f, -1.0f };

    constexpr quaternion PK_QUATERNION_IDENTITY = { 1.0f, 0.0f, 0.0f, 0.0f };

    constexpr float4x4 PK_FLOAT3X4_IDENTITY = float3x4(1.0f);
    constexpr float4x4 PK_FLOAT4X4_IDENTITY = float4x4(1.0f);

    constexpr color PK_COLOR_WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };
    constexpr color PK_COLOR_GRAY = { 0.5f, 0.5f, 0.5f, 1.0f };
    constexpr color PK_COLOR_BLACK = { 0.0f, 0.0f, 0.0f, 1.0f };
    constexpr color PK_COLOR_CLEAR = { 0.0f, 0.0f, 0.0f, 0.0f };
    constexpr color PK_COLOR_RED = { 1.0f, 0.0f, 0.0f, 1.0f };
    constexpr color PK_COLOR_GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
    constexpr color PK_COLOR_BLUE = { 0.0f, 0.0f, 1.0f, 1.0f };
    constexpr color PK_COLOR_CYAN = { 0.0f, 1.0f, 1.0f, 1.0f };
    constexpr color PK_COLOR_MAGENTA = { 1.0f, 0.0f, 1.0f, 1.0f };
    constexpr color PK_COLOR_YELLOW = { 1.0f, 1.0f, 0.0f, 1.0f };

    constexpr color32 PK_COLOR32_WHITE = { 255, 255, 255, 255 };
    constexpr color32 PK_COLOR32_GRAY = { 128, 128, 128, 255 };
    constexpr color32 PK_COLOR32_BLACK = { 0,   0,   0, 255 };
    constexpr color32 PK_COLOR32_CLEAR = { 0,   0,   0,   0 };
    constexpr color32 PK_COLOR32_RED = { 255,   0,   0, 255 };
    constexpr color32 PK_COLOR32_GREEN = { 0, 255,   0, 255 };
    constexpr color32 PK_COLOR32_BLUE = { 0,   0, 255, 255 };
    constexpr color32 PK_COLOR32_CYAN = { 0, 255, 255, 255 };
    constexpr color32 PK_COLOR32_MAGENTA = { 255,   0, 255, 255 };
    constexpr color32 PK_COLOR32_YELLOW = { 255, 255,   0, 255 };

    constexpr float PK_FLOAT_PI = 3.1415926535f;
    constexpr float PK_FLOAT_TWO_PI = 6.2831853071f;
    constexpr float PK_FLOAT_FOUR_PI = 12.566370614f;
    constexpr float PK_FLOAT_INV_PI = 0.3183098861f;
    constexpr float PK_FLOAT_INV_TWO_PI = 0.1591549430f;
    constexpr float PK_FLOAT_INV_FOUR_PI = 0.0795774715f;
    constexpr float PK_FLOAT_HALF_PI = 1.5707963267f;
    constexpr float PK_FLOAT_INV_HALF_PI = 0.6366197723f;
    constexpr float PK_FLOAT_SQRT_PI = 1.7724538509f;
    constexpr float PK_FLOAT_TWO_SQRT2 = 2.8284271247f;
    constexpr float PK_FLOAT_SQRT2 = 1.4142135623f;
    constexpr float PK_FLOAT_INV_SQRT2 = 0.7071067811f;
    constexpr float PK_FLOAT_DEG2RAD = 0.0174532924F;
    constexpr float PK_FLOAT_RAD2DEG = 57.29578F;
    constexpr float PK_CLIPZ_NEAR = 1.0f;
    constexpr float PK_CLIPZ_FAR = 0.0f;
    constexpr float PK_HALF_MAX = 65504.0;
    constexpr float PK_HALF_MAX_MINUS1 = 65472.0;

    struct FrustumPlanes
    {
        float4 left;
        float4 right;
        float4 top;
        float4 bottom;
        float4 far;
        float4 near;
        inline float4& operator[](size_t i) { return (&left)[i]; }
        constexpr float4* array_ptr() { return &left; }
    };

    struct ShadowCascadeCreateInfo
    {
        float4x4 worldToLocal;
        float4x4 clipToWorld;
        float* splitPlanes;
        float nearPlaneOffset;
        uint32_t resolution;
        uint32_t count;
    };

    struct BoundingBox
    {
        float3 min;
        float3 max;

        float3 GetCenter() const { return min + (max - min) * 0.5f; }
        float3 GetExtents() const { return (max - min) * 0.5f; }
        float GetWidth() const { return max.x - min.x; }
        float GetHeight() const { return max.y - min.y; }
        float GetDepth() const { return max.z - min.z; }

        BoundingBox() : min(PK_FLOAT3_ZERO), max(PK_FLOAT3_ZERO) {}
        BoundingBox(const float3& _min, const float3& _max) : min(_min), max(_max) {}
        inline static BoundingBox MinMax(const float3& min, const float3& max) { return BoundingBox(min, max); }
        inline static BoundingBox CenterExtents(const float3& center, const float3& extents) { return BoundingBox(center - extents, center + extents); }
        inline static BoundingBox GetMinBounds()
        {
            constexpr const float fmi = -std::numeric_limits<float>().max();
            constexpr const float fma = std::numeric_limits<float>().max();
            return BoundingBox({ fma, fma, fma }, { fmi, fmi, fmi });
        }
    };
}
