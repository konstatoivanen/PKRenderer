#pragma once
#include "Core/Math/Math.h"

namespace PK::App
{
    struct ComponentBounds
    {
        BoundingBox localAABB;
        BoundingBox worldAABB;
        virtual ~ComponentBounds() = default;
    };
}