#pragma once
#include "Core/ECS/IEntityView.h"
#include "App/ECS/ComponentTransform.h"

namespace PK::App
{
    struct EntityViewLightSphereTransforms : public IEntityView
    {
        ComponentTransform* transformMesh;
        ComponentTransform* transformLight;
    };
}
