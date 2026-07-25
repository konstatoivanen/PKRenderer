#pragma once
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentBounds.h"
#include "App/ECS/ComponentScenePrimitive.h"

namespace PK::App
{
    struct EntityViewScenePrimitive
    {
        EntityComponentRef<ComponentBounds> bounds;
        EntityComponentRef<ComponentScenePrimitive> primitive;
    };
}
