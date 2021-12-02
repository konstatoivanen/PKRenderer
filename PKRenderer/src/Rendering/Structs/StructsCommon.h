#pragma once
#include "Math/PKMath.h"

namespace PK::Rendering::Structs
{
    using namespace PK::Math;

    enum class LightType : uint
    {
        Point = 0,
        Spot = 1,
        Directional = 2,
        TypeCount
    };

    enum class LightCookie : uint
    {
        Circle0 = 0,
        Circle1 = 1,
        Circle2 = 2,
        Square0 = 3,
        Square1 = 4,
        Square2 = 5,
        Triangle = 6,
        Star = 7,
        NoCookie = 0xFFFFFFFF
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
        uint offset = 0;
        uint count = 0;
    };
}