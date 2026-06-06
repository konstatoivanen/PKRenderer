#pragma once
#include "Core/ECS/IEntityImplementer.h"
#include "Core/ECS/EntityFactory.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentBounds.h"
#include "App/ECS/ComponentProjection.h"
#include "App/ECS/ComponentRenderView.h"
#include "App/ECS/ComponentViewInput.h"
#include "App/ECS/ComponentTime.h"
#include "App/ECS/ComponentFlyCamera.h"

namespace PK::App
{
    struct ImplementerFlyCamera : public IEntityImplementer,
        public ComponentTransform,
        public ComponentBounds,
        public ComponentProjection,
        public ComponentRenderView,
        public ComponentViewInput,
        public ComponentTime,
        public ComponentFlyCamera
    {
    };

    struct EntityFlyCamera : EntityFactory<EntityFlyCamera>
    {
        FixedString16 name;
        uint4 desiredRect;
        bool isWindowTarget;
        float3 position;
        float3 rotation;
        float moveSpeed;
        float fieldOfView;
        float zNear;
        float zFar;
        float moveSmoothing;
        float rotationSmoothing;
        float sensitivity;
        RenderViewSettings* settings;
    };
}
