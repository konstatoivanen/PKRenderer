#pragma once
#include "Core/ECS/IEntityView.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentRenderView.h"
#include "App/ECS/ComponentProjection.h"
#include "App/ECS/ComponentViewInput.h"
#include "App/ECS/ComponentTime.h"

namespace PK::App
{
    struct EntityViewRenderView : public IEntityView
    {
        ComponentTransform* transform;
        ComponentProjection* projection;
        ComponentRenderView* renderView;
        ComponentViewInput* input;
        ComponentTime* time;
    };
}
