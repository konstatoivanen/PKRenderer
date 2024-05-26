#pragma once
#include "Core/ECS/IEntityView.h"
#include "App/ECS/ComponentBounds.h"
#include "App/ECS/ComponentScenePrimitive.h"

namespace PK::App
{
    struct EntityViewScenePrimitive : public IEntityView
    {
        ComponentBounds* bounds;
        ComponentScenePrimitive* primitive;
    };
}