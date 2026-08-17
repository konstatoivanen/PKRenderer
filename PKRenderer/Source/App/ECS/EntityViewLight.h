#pragma once
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentLight.h"
#include "App/ECS/ComponentScenePrimitive.h"
#include "App/ECS/ComponentBounds.h"

namespace PK::App
{
    struct EntityViewLight
    {
        uint32_t* entityId;
        ComponentTransform* transform;
        ComponentBounds* bounds;
        ComponentLight* light;
        ComponentScenePrimitive* primitive;
    };
}
