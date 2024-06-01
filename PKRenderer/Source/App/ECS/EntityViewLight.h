#pragma once
#include "Core/ECS/IEntityView.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentLight.h"
#include "App/ECS/ComponentScenePrimitive.h"
#include "App/ECS/ComponentBounds.h"

namespace PK::App
{
    struct EntityViewLight : public IEntityView
    {
        ComponentTransform* transform;
        ComponentBounds* bounds;
        ComponentLight* light;
        ComponentScenePrimitive* primitive;
    };
}