#pragma once
#include "Core/Math/Math.h"
#include "Core/ECS/NotSerialized.h"

namespace PK::App
{
    struct ComponentBounds
    {
        AABB<float3> localAABB;
        PK_ECS_PRIVATE_FIELDS
        AABB<float3> worldAABB;
    };
}
