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
    struct EntityViewRenderView : public IEntityView
    {
        EntityComponentRef<ComponentTransform> transform;
        EntityComponentRef<ComponentProjection> projection;
        EntityComponentRef<ComponentRenderView> renderView;
        EntityComponentRef<ComponentViewInput> input;
        EntityComponentRef<ComponentTime> time;
    };
}
