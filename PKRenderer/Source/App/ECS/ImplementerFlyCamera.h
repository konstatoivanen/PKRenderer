#pragma once
#include "Core/ECS/IEntityImplementer.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentBounds.h"
#include "App/ECS/ComponentProjection.h"
#include "App/ECS/ComponentRenderView.h"
#include "App/ECS/ComponentFlyCamera.h"

namespace PK::App
{
    struct ImplementerFlyCamera : public IEntityImplementer,
        public ComponentTransform,
        public ComponentBounds,
        public ComponentProjection,
        public ComponentRenderView,
        public ComponentFlyCamera
    {
    };
}