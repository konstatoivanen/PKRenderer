#pragma once
#include "Vector.h"
#include "Quaternion.h"
#include "Bounds.h"

namespace PK
{
    constexpr float2 PK_FLOAT2_ZERO = { 0.0f, 0.0f };
    constexpr float3 PK_FLOAT3_ZERO = { 0.0f, 0.0f, 0.0f };
    constexpr float4 PK_FLOAT4_ZERO = { 0.0f, 0.0f, 0.0f, 0.0f };

    constexpr float2 PK_FLOAT2_MAX = { FLT_MAX, FLT_MAX };
    constexpr float3 PK_FLOAT3_MAX = { FLT_MAX, FLT_MAX, FLT_MAX };
    constexpr float4 PK_FLOAT4_MAX = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };

    constexpr ushort2 PK_USHORT2_ZERO = { 0, 0 };
    constexpr ushort3 PK_USHORT3_ZERO = { 0, 0, 0 };
    constexpr ushort4 PK_USHORT4_ZERO = { 0, 0, 0, 0 };

    constexpr ushort2 PK_USHORT2_MAX = { ~0u, ~0u };
    constexpr ushort3 PK_USHORT3_MAX = { ~0u, ~0u, ~0u };
    constexpr ushort4 PK_USHORT4_MAX = { ~0u, ~0u, ~0u, ~0u };

    constexpr short2 PK_SHORT2_ZERO = { 0, 0 };
    constexpr short3 PK_SHORT3_ZERO = { 0, 0, 0 };
    constexpr short4 PK_SHORT4_ZERO = { 0, 0, 0, 0 };

    constexpr short2 PK_SHORT2_MAX = { 32767, 32767 };
    constexpr short3 PK_SHORT3_MAX = { 32767, 32767, 32767 };
    constexpr short4 PK_SHORT4_MAX = { 32767, 32767, 32767, 32767 };

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

    constexpr int2 PK_INT2_MINUS_ONE = { -1, -1 };
    constexpr int3 PK_INT3_MINUS_ONE = { -1, -1, -1 };
    constexpr int4 PK_INT4_MINUS_ONE = { -1, -1, -1, -1 };

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

    constexpr quaternion PK_QUATERNION_IDENTITY = { 0.0f, 0.0f, 0.0f, 1.0f };

    constexpr float3x4 PK_FLOAT3X4_IDENTITY = float3x4(1.0f);
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

    constexpr AABB<float3> PK_FLOAT3_MIN_AABB = { PK_FLOAT3_MAX, -PK_FLOAT3_MAX };

    struct FrustumPlanes : math::convex<float,6>
    {
        constexpr FrustumPlanes() = default;
        constexpr FrustumPlanes(const FrustumPlanes& p) = default;
        constexpr FrustumPlanes(const math::convex<float,6>& p) : math::convex<float, 6>(p) {};
        float4& left() { return planes[0]; }
        float4& right() { return planes[1]; }
        float4& top() { return planes[2]; }
        float4& bottom() { return planes[3]; }
        float4& far() { return planes[4]; }
        float4& near() { return planes[5]; }
        const float4& left() const { return planes[0]; }
        const float4& right() const { return planes[1]; }
        const float4& top() const { return planes[2]; }
        const float4& bottom() const { return planes[3]; }
        const float4& far() const { return planes[4]; }
        const float4& near() const { return planes[5]; }
    };

    struct ShadowCascadeCreateInfo
    {
        float4x4 worldToLocal;
        float4x4 clipToWorld;
        float* splitPlanes;
        float nearPlaneOffset;
        float padding;
        uint32_t resolution;
        uint32_t count;
    };
}
