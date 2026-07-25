#pragma once
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentLight.h"
#include "App/ECS/ComponentScenePrimitive.h"
#include "App/ECS/ComponentBounds.h"

namespace PK::App
{
    struct EntityViewLight
    {
        EntityComponentRef<ComponentTransform> transform;
        EntityComponentRef<ComponentBounds> bounds;
        EntityComponentRef<ComponentLight> light;
        EntityComponentRef<ComponentScenePrimitive> primitive;
    };
}
