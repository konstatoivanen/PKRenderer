#pragma once
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentRenderView.h"
#include "App/ECS/ComponentProjection.h"
#include "App/ECS/ComponentViewInput.h"
#include "App/ECS/ComponentTime.h"

namespace PK::App
{
    struct EntityViewRenderView
    {
        uint32_t* entityId;
        ComponentTransform* transform;
        ComponentProjection* projection;
        ComponentRenderView* renderView;
        ComponentViewInput* input;
        ComponentTime* time;
    };
}
