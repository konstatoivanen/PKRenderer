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

    struct PK_Transform
    {
        float4x4 localToWorld;
        float4x4 worldToLocal;
    };

    struct FrustumTileAABB
    {
        float4 minPoint;
        float4 maxPoint;
    };

    struct LightTile
    {
        uint offset = 0;
        uint count = 0;
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

    struct PKRawLight
    {
        float4 color;
        float4 position;
        uint shadowmap_index = 0;
        uint projection_index = 0;
        uint cookie_index = 0;
        uint type = 0;
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