#pragma once
#include "Core/ECS/IEntityView.h"
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentLight.h"
#include "App/ECS/ComponentScenePrimitive.h"
#include "App/ECS/ComponentBounds.h"

namespace PK::App
{
    PK_ECS_VIEW_BEGIN(EntityViewLight)
        PK_ECS_VIEW_COMPONENT(ComponentTransform, transform)
        PK_ECS_VIEW_COMPONENT(ComponentBounds, bounds)
        PK_ECS_VIEW_COMPONENT(ComponentLight, light)
        PK_ECS_VIEW_COMPONENT(ComponentScenePrimitive, primitive)
    PK_ECS_VIEW_END()
}
