#pragma once
#include "Math/Types.h"

namespace PK::ECS::Components
{
    struct Bounds
    {
        PK::Math::BoundingBox localAABB;
        PK::Math::BoundingBox worldAABB;
        virtual ~Bounds() = default;
    };
}