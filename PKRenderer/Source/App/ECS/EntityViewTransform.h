#pragma once
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentBounds.h"

namespace PK::App
{
    struct EntityViewTransform
    {
        uint32_t* entityId;
        ComponentTransform* transform;
        ComponentBounds* bounds;
    };
}
