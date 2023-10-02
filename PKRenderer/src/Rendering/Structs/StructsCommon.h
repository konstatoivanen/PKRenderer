#pragma once
#include "Math/Types.h"

namespace PK::Rendering::Structs
{
    struct PK_Draw
    {
        Math::uint material;
        Math::uint transfrom;
        Math::uint mesh;
        Math::uint userdata;
    };

    // Packed into float4, float4, uint4
    struct alignas(16) LightPacked
    {
        Math::float3 position = Math::PK_FLOAT3_ZERO;
        float radius = 0.0f;
        Math::float3 color = Math::PK_FLOAT3_ZERO;
        float angle = 0.0f;
        Math::ushort shadowIndex = 0xFFFFu;
        Math::ushort matrixIndex = 0u;
        Math::ushort type = 0xFFFFu;
        Math::ushort cookie = 0xFFFFu;
        Math::uint direction = 0u;
        float sourceRadius = 0.0f;
    };

    struct Vertex_Simple
    {
        Math::float3 position;
        Math::float2 texcoord;
    };

    struct Vertex_Full
    {
        Math::float3 position;
        Math::float3 normal;
        Math::float4 tangent;
        Math::float2 texcoord;
    };

    struct IndexRange
    {
        size_t offset = 0;
        size_t count = 0;

        constexpr bool operator < (const IndexRange& other) const
        {
            return offset != other.offset ? offset < other.offset : count < other.count;
        }

        constexpr bool operator == (const IndexRange& other) const
        {
            return offset == other.offset && count == other.count;
        }
    };
}