#pragma once
#include "Core/ECS/EntityFactory.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewRenderView.h"
#include "App/ECS/EntityViewFlyCamera.h"

namespace PK::App
{
    struct EntityFlyCamera
    {
        struct Descriptor
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

        uint32_t* entityId;
        ComponentTransform* transform;
        ComponentBounds* bounds;
        ComponentProjection* projection;
        ComponentRenderView* renderView;
        ComponentViewInput* viewInput;
        ComponentTime* time;
        ComponentFlyCamera* flyCamera;

        static void OnCreate(EntityDatabase* entityDb, EntityFlyCamera& entity, const Descriptor& descriptor);

    };
}
