#pragma once
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentTransform.h"

namespace PK::App
{
    struct EntityViewLightSphereTransforms
    {
        EntityComponentRef<ComponentTransform> transformMesh;
        EntityComponentRef<ComponentTransform> transformLight;
    };
}
