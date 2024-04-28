#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/ComponentTransform.h"

namespace PK::ECS
{
    struct EntityViewLightSphereTransforms : public IEntityView
    {
        ComponentTransform* transformMesh;
        ComponentTransform* transformLight;
    };
}