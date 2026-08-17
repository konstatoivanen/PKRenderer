#pragma once
#include "App/ECS/ComponentBounds.h"
#include "App/ECS/ComponentScenePrimitive.h"

namespace PK::App
{
    struct EntityViewScenePrimitive
    {
        uint32_t* entityId;
        ComponentBounds* bounds;
        ComponentScenePrimitive* primitive;
    };
}
