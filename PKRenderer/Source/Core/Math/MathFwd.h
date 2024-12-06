#pragma once
#define GLM_FORCE_SWIZZLE 
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/fwd.hpp>

namespace PK
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
    typedef glm::dmat3x4 double3x4;
    typedef glm::dmat4x4 double4x4;

    typedef glm::i16vec2 short2;
    typedef glm::i16vec3 short3;
    typedef glm::i16vec4 short4;

    typedef glm::u16vec2 ushort2;
    typedef glm::u16vec3 ushort3;
    typedef glm::u16vec4 ushort4;

    typedef glm::u16mat2x2 ushort2x2;
    typedef glm::u16mat3x3 ushort3x3;
    typedef glm::u16mat3x4 ushort3x4;
    typedef glm::u16mat4x4 ushort4x4;

    typedef glm::lowp_i8vec3 sbyte3;
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

    struct FrustumPlanes;
    struct ShadowCascadeCreateInfo;
    struct BoundingBox;
}
