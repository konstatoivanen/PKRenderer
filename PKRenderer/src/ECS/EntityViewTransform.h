#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/ComponentTransform.h"
#include "ECS/ComponentBounds.h"

namespace PK::ECS
{
    struct EntityViewTransform : public IEntityView
    {
        ComponentTransform* transform;
        ComponentBounds* bounds;
    };
}