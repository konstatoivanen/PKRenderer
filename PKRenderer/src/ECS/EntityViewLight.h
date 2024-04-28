#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/ComponentTransform.h"
#include "ECS/ComponentLight.h"
#include "ECS/ComponentScenePrimitive.h"
#include "ECS/ComponentBounds.h"

namespace PK::ECS
{
    struct EntityViewLight : public IEntityView
    {
        ComponentTransform* transform;
        ComponentBounds* bounds;
        ComponentLight* light;
        ComponentScenePrimitive* primitive;
    };
}