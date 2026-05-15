#pragma once
#include "Core/ECS/IEntityView.h"
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentTransform.h"

namespace PK::App
{
    struct EntityViewLightSphereTransforms : public IEntityView
    {
        EntityComponentRef<ComponentTransform> transformMesh;
        EntityComponentRef<ComponentTransform> transformLight;
    };
}
