#pragma once
#include "Core/ECS/IEntityView.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentProjection.h"
#include "App/ECS/ComponentViewInput.h"
#include "App/ECS/ComponentTime.h"
#include "App/ECS/ComponentFlyCamera.h"

namespace PK::App
{
    PK_ECS_VIEW_BEGIN(EntityViewFlyCamera)
        PK_ECS_VIEW_COMPONENT(ComponentTransform, transform)
        PK_ECS_VIEW_COMPONENT(ComponentProjection, projection)
        PK_ECS_VIEW_COMPONENT(ComponentViewInput, input)
        PK_ECS_VIEW_COMPONENT(ComponentTime, time)
        PK_ECS_VIEW_COMPONENT(ComponentFlyCamera, flyCamera)
    PK_ECS_VIEW_END()
}
