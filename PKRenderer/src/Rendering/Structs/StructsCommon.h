#pragma once
#include "Math/Types.h"

namespace PK::Rendering::Structs
{
    using namespace PK::Math;

    struct PK_Draw
    {
        uint material;
        uint transfrom;
        uint mesh;
        uint clipInfo;
    };

    struct PK_Light
    {
        float4 position;
        float4 color;
        uint4 indices;
    };

    struct Vertex_Simple
    {
        float3 position;
        float2 texcoord;
    };

    struct Vertex_Full
    {
        float3 position;
        float3 normal;
        float4 tangent;
        float2 texcoord;
    };

    struct IndexRange
    {
        size_t offset = 0;
        size_t count = 0;

        constexpr bool operator < (const IndexRange& other) const
        {
            return offset != other.offset ? offset < other.offset : count < other.count;
        }
    };
}