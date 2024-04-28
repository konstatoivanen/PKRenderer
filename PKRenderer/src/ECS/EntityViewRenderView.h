#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/ComponentTransform.h"
#include "ECS/ComponentRenderView.h"
#include "ECS/ComponentProjection.h"

namespace PK::ECS
{
    struct EntityViewRenderView : public IEntityView
    {
        ComponentTransform* transform;
        ComponentProjection* projection;
        ComponentRenderView* renderView;
    };
}