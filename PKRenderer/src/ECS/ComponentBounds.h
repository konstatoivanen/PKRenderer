#pragma once
#include "Math/Types.h"

namespace PK::ECS
{
    struct ComponentBounds
    {
        PK::Math::BoundingBox localAABB;
        PK::Math::BoundingBox worldAABB;
        virtual ~ComponentBounds() = default;
    };
}