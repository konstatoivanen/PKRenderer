#pragma once
#include "Core/ECS/IEntityView.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentBounds.h"

namespace PK::App
{
    struct EntityViewTransform : public IEntityView
    {
        ComponentTransform* transform;
        ComponentBounds* bounds;
    };
}
