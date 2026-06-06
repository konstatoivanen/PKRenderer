#pragma once
#include "Core/ECS/IEntityView.h"
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentBounds.h"
#include "App/ECS/ComponentScenePrimitive.h"

namespace PK::App
{
    PK_ECS_VIEW_BEGIN(EntityViewScenePrimitive)
        PK_ECS_VIEW_COMPONENT(ComponentBounds, bounds)
        PK_ECS_VIEW_COMPONENT(ComponentScenePrimitive, primitive)
    PK_ECS_VIEW_END()
}
