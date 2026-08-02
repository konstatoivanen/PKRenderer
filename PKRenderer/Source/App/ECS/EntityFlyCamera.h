#pragma once
#include "Core/ECS/EntityFactory.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewRenderView.h"
#include "App/ECS/EntityViewFlyCamera.h"

namespace PK::App
{
    struct ImplementerFlyCamera : public TEntityImplementer<
        ComponentTransform,
        ComponentBounds,
        ComponentProjection,
        ComponentRenderView,
        ComponentViewInput,
        ComponentTime,
        ComponentFlyCamera>
    {
    };

    struct EntityFlyCamera
    {
        using TImplementers = Tuple<ImplementerFlyCamera*>;
        using TViews = Tuple<EntityViewTransform, EntityViewRenderView, EntityViewFlyCamera>;

        //static void OnDeserialize(EntityDatabase* entityDb, const EGID& egid, SerialNodeRead node, TImplementers& implementers) = delete;
        //static void OnSerialize(EntityDatabase* entityDb, const EGID& egid, SerialNodeWrite node) = delete;
        static void OnCreate(EntityDatabase* entityDb, const EGID& egid, const EntityFlyCamera& descriptor, TImplementers& implementers);


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
