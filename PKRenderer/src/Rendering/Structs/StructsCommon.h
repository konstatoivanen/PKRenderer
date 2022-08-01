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

    struct PK_Light
    {
        Math::float4 position;
        Math::float4 color;
        Math::uint4 indices;
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