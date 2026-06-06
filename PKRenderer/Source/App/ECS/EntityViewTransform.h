#pragma once
#include "Core/ECS/IEntityView.h"
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentBounds.h"

namespace PK::App
{
    PK_ECS_VIEW_BEGIN(EntityViewTransform)
        PK_ECS_VIEW_COMPONENT(ComponentTransform, transform)
        PK_ECS_VIEW_COMPONENT(ComponentBounds, bounds)
    PK_ECS_VIEW_END()
}
