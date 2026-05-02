#pragma once
#include "Core/Math/Math.h"

namespace PK::App
{
    struct ComponentBounds
    {
        AABB<float3> localAABB;
        AABB<float3> worldAABB;
        virtual ~ComponentBounds() = default;
    };
}
