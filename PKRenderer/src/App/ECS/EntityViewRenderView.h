#pragma once
#include "Core/ECS/IEntityView.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentRenderView.h"
#include "App/ECS/ComponentProjection.h"

namespace PK::App
{
    struct EntityViewRenderView : public IEntityView
    {
        ComponentTransform* transform;
        ComponentProjection* projection;
        ComponentRenderView* renderView;
    };
}