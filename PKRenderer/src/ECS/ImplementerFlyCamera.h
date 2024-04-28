#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/ComponentTransform.h"
#include "ECS/ComponentBounds.h"
#include "ECS/ComponentProjection.h"
#include "ECS/ComponentRenderView.h"
#include "ECS/ComponentFlyCamera.h"

namespace PK::ECS
{
    struct ImplementerFlyCamera : public IImplementer,
        public ComponentTransform,
        public ComponentBounds,
        public ComponentProjection,
        public ComponentRenderView,
        public ComponentFlyCamera
    {
    };
}