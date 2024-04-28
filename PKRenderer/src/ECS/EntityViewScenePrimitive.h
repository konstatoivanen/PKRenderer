#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/ComponentBounds.h"
#include "ECS/ComponentScenePrimitive.h"

namespace PK::ECS
{
    struct EntityViewScenePrimitive : public IEntityView
    {
        ComponentBounds* bounds;
        ComponentScenePrimitive* primitive;
    };
}