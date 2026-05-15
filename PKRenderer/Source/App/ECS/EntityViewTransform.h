#pragma once
#include "Core/ECS/IEntityView.h"
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentBounds.h"

namespace PK::App
{
    struct EntityViewTransform : public IEntityView
    {
        EntityComponentRef<ComponentTransform> transform;
        EntityComponentRef<ComponentBounds> bounds;
    };
}
