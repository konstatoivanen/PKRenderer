#pragma once
#include "Math/Types.h"

namespace PK::ECS::Components
{
    using namespace PK::Math;

    struct Bounds
    {
        BoundingBox localAABB;
        BoundingBox worldAABB;
        virtual ~Bounds() = default;
    };
}