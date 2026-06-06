#pragma once
#include "Core/ECS/IEntityView.h"
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentRenderView.h"
#include "App/ECS/ComponentProjection.h"
#include "App/ECS/ComponentViewInput.h"
#include "App/ECS/ComponentTime.h"

namespace PK::App
{
    PK_ECS_VIEW_BEGIN(EntityViewRenderView)
        PK_ECS_VIEW_COMPONENT(ComponentTransform, transform)
        PK_ECS_VIEW_COMPONENT(ComponentProjection, projection)
        PK_ECS_VIEW_COMPONENT(ComponentRenderView, renderView)
        PK_ECS_VIEW_COMPONENT(ComponentViewInput, input)
        PK_ECS_VIEW_COMPONENT(ComponentTime, time)
    PK_ECS_VIEW_END()
}
