#pragma once
#include <stdint.h>

// SIMD defines
#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64) || defined(__SSE2__)
    #define PK_MATH_SIMD_SSE2 1
    #include <emmintrin.h>
    #if defined(__SSE3__)
        #define PK_MATH_SIMD_SSE3 1
        #include <pmmintrin.h>
    #endif
    #if defined(__SSSE3__)
        #define PK_MATH_SIMD_SSSE3 1
        #include <tmmintrin.h>
    #endif
    #if defined(__SSE4_1__)
        #define PK_MATH_SIMD_SSE4_1 1
        #include <smmintrin.h>
    #endif
    #if defined(__SSE4_2__)
        #define PK_MATH_SIMD_SSE4_2 1
        #if defined(__clang__)
            #include <popcntintrin.h>
        #endif
        #include <nmmintrin.h>
    #endif
#endif

#if defined(_M_ARM) || defined(__ARM_NEON__) || defined(__ARM_NEON)
    #define PK_MATH_SIMD_NEON 1
    #include "neon.h"
#endif

#define PK_MATH_SIMD (PK_MATH_SIMD_SSE2 || PK_MATH_SIMD_SSE3 || PK_MATH_SIMD_SSSE3 || PK_MATH_SIMD_SSE4_1 || PK_MATH_SIMD_SSE4_2 || PK_MATH_SIMD_NEON)

namespace PK
{
    namespace math
    {
        template<typename T, int N> struct vector {};
        template<typename T, int C, int R> struct matrix {};
    }

    typedef uint16_t ushort;
    typedef uint32_t uint;
    typedef uint64_t ulong;
    typedef uint8_t byte;
    typedef int8_t sbyte;

    typedef math::vector<float,2> float2;
    typedef math::vector<float,3> float3;
    typedef math::vector<float,4> float4;
    typedef math::vector<double,2> double2;
    typedef math::vector<double,3> double3;
    typedef math::vector<double,4> double4;

    typedef math::vector<bool,2> bool2;
    typedef math::vector<bool,3> bool3;
    typedef math::vector<bool,4> bool4;

    typedef math::vector<uint8_t,2> byte2;
    typedef math::vector<uint8_t,3> byte3;
    typedef math::vector<uint8_t,4> byte4;
    typedef math::vector<uint16_t,2> ushort2;
    typedef math::vector<uint16_t,3> ushort3;
    typedef math::vector<uint16_t,4> ushort4;
    typedef math::vector<uint32_t,2> uint2;
    typedef math::vector<uint32_t,3> uint3;
    typedef math::vector<uint32_t,4> uint4;
    typedef math::vector<uint64_t,2> ulong2;
    typedef math::vector<uint64_t,3> ulong3;
    typedef math::vector<uint64_t,4> ulong4;

    typedef math::vector<int8_t, 2> sbyte2;
    typedef math::vector<int8_t, 3> sbyte3;
    typedef math::vector<int8_t, 4> sbyte4;
    typedef math::vector<int16_t, 2> short2;
    typedef math::vector<int16_t, 3> short3;
    typedef math::vector<int16_t, 4> short4;
    typedef math::vector<int32_t, 2> int2;
    typedef math::vector<int32_t, 3> int3;
    typedef math::vector<int32_t, 4> int4;
    typedef math::vector<int64_t, 2> long2;
    typedef math::vector<int64_t, 3> long3;
    typedef math::vector<int64_t, 4> long4;

    typedef math::matrix<float,2,2> float2x2;
    typedef math::matrix<float,2,3> float2x3;
    typedef math::matrix<float,2,4> float2x4;
    typedef math::matrix<float,3,2> float3x2;
    typedef math::matrix<float,3,3> float3x3;
    typedef math::matrix<float,3,4> float3x4;
    typedef math::matrix<float,4,2> float4x2;
    typedef math::matrix<float,4,3> float4x3;
    typedef math::matrix<float,4,4> float4x4;

    typedef math::matrix<double,2,2> double2x2;
    typedef math::matrix<double,2,3> double2x3;
    typedef math::matrix<double,2,4> double2x4;
    typedef math::matrix<double,3,2> double3x2;
    typedef math::matrix<double,3,3> double3x3;
    typedef math::matrix<double,3,4> double3x4;
    typedef math::matrix<double,4,2> double4x2;
    typedef math::matrix<double,4,3> double4x3;
    typedef math::matrix<double,4,4> double4x4;

    typedef math::matrix<bool,2,2> bool2x2;
    typedef math::matrix<bool,2,3> bool2x3;
    typedef math::matrix<bool,2,4> bool2x4;
    typedef math::matrix<bool,3,2> bool3x2;
    typedef math::matrix<bool,3,3> bool3x3;
    typedef math::matrix<bool,3,4> bool3x4;
    typedef math::matrix<bool,4,2> bool4x2;
    typedef math::matrix<bool,4,3> bool4x3;
    typedef math::matrix<bool,4,4> bool4x4;

    typedef math::matrix<uint8_t,2,2> byte2x2;
    typedef math::matrix<uint8_t,2,3> byte2x3;
    typedef math::matrix<uint8_t,2,4> byte2x4;
    typedef math::matrix<uint8_t,3,2> byte3x2;
    typedef math::matrix<uint8_t,3,3> byte3x3;
    typedef math::matrix<uint8_t,3,4> byte3x4;
    typedef math::matrix<uint8_t,4,2> byte4x2;
    typedef math::matrix<uint8_t,4,3> byte4x3;
    typedef math::matrix<uint8_t,4,4> byte4x4;

    typedef math::matrix<uint16_t,2,2> ushort2x2;
    typedef math::matrix<uint16_t,2,3> ushort2x3;
    typedef math::matrix<uint16_t,2,4> ushort2x4;
    typedef math::matrix<uint16_t,3,2> ushort3x2;
    typedef math::matrix<uint16_t,3,3> ushort3x3;
    typedef math::matrix<uint16_t,3,4> ushort3x4;
    typedef math::matrix<uint16_t,4,2> ushort4x2;
    typedef math::matrix<uint16_t,4,3> ushort4x3;
    typedef math::matrix<uint16_t,4,4> ushort4x4;

    typedef math::matrix<uint32_t,2,2> uint2x2;
    typedef math::matrix<uint32_t,2,3> uint2x3;
    typedef math::matrix<uint32_t,2,4> uint2x4;
    typedef math::matrix<uint32_t,3,2> uint3x2;
    typedef math::matrix<uint32_t,3,3> uint3x3;
    typedef math::matrix<uint32_t,3,4> uint3x4;
    typedef math::matrix<uint32_t,4,2> uint4x2;
    typedef math::matrix<uint32_t,4,3> uint4x3;
    typedef math::matrix<uint32_t,4,4> uint4x4;

    typedef math::matrix<uint64_t,2,2> ulong2x2;
    typedef math::matrix<uint64_t,2,3> ulong2x3;
    typedef math::matrix<uint64_t,2,4> ulong2x4;
    typedef math::matrix<uint64_t,3,2> ulong3x2;
    typedef math::matrix<uint64_t,3,3> ulong3x3;
    typedef math::matrix<uint64_t,3,4> ulong3x4;
    typedef math::matrix<uint64_t,4,2> ulong4x2;
    typedef math::matrix<uint64_t,4,3> ulong4x3;
    typedef math::matrix<uint64_t,4,4> ulong4x4;

    typedef math::matrix<int8_t,2,2> sbyte2x2;
    typedef math::matrix<int8_t,2,3> sbyte2x3;
    typedef math::matrix<int8_t,2,4> sbyte2x4;
    typedef math::matrix<int8_t,3,2> sbyte3x2;
    typedef math::matrix<int8_t,3,3> sbyte3x3;
    typedef math::matrix<int8_t,3,4> sbyte3x4;
    typedef math::matrix<int8_t,4,2> sbyte4x2;
    typedef math::matrix<int8_t,4,3> sbyte4x3;
    typedef math::matrix<int8_t,4,4> sbyte4x4;

    typedef math::matrix<int16_t,2,2> short2x2;
    typedef math::matrix<int16_t,2,3> short2x3;
    typedef math::matrix<int16_t,2,4> short2x4;
    typedef math::matrix<int16_t,3,2> short3x2;
    typedef math::matrix<int16_t,3,3> short3x3;
    typedef math::matrix<int16_t,3,4> short3x4;
    typedef math::matrix<int16_t,4,2> short4x2;
    typedef math::matrix<int16_t,4,3> short4x3;
    typedef math::matrix<int16_t,4,4> short4x4;

    typedef math::matrix<int32_t,2,2> int2x2;
    typedef math::matrix<int32_t,2,3> int2x3;
    typedef math::matrix<int32_t,2,4> int2x4;
    typedef math::matrix<int32_t,3,2> int3x2;
    typedef math::matrix<int32_t,3,3> int3x3;
    typedef math::matrix<int32_t,3,4> int3x4;
    typedef math::matrix<int32_t,4,2> int4x2;
    typedef math::matrix<int32_t,4,3> int4x3;
    typedef math::matrix<int32_t,4,4> int4x4;

    typedef math::matrix<int64_t,2,2> long2x2;
    typedef math::matrix<int64_t,2,3> long2x3;
    typedef math::matrix<int64_t,2,4> long2x4;
    typedef math::matrix<int64_t,3,2> long3x2;
    typedef math::matrix<int64_t,3,3> long3x3;
    typedef math::matrix<int64_t,3,4> long3x4;
    typedef math::matrix<int64_t,4,2> long4x2;
    typedef math::matrix<int64_t,4,3> long4x3;
    typedef math::matrix<int64_t,4,4> long4x4;

    typedef byte4 color32;
    typedef float4 color;
    typedef float4 quaternion;
    struct FrustumPlanes;
    struct ShadowCascadeCreateInfo;
    struct BoundingBox;

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
}
