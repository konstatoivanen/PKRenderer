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
        Math::float3 position;
        float radius;
        Math::float3 color;
        float angle;
        Math::ushort shadowIndex;
        Math::ushort matrixIndex;
        Math::ushort type;
        Math::ushort cookie;
        Math::uint direction;
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