#pragma once
#include "Core/ECS/EntityComponentSerializable.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "App/ECS/EntityViewLight.h"

namespace PK
{
    struct EntityDatabase;
    struct EntityVisitorsView;
}

namespace PK::App
{
    struct EntityLight
    {
        struct Descriptor
        {
            FixedString64 entityName;
            AssetID sceneId;

            IESProfileRef IESProfile;
            float3 position;
            float3 rotation;
            color color;
            float angle;
            float radius;
            float sourceRadius;
            LightType type;
            bool useIESCandelas;
            bool castShadow;
        };

        uint32_t* entityId;
        ComponentTransform* transform;
        ComponentBounds* bounds;
        ComponentScenePrimitive* primitive;
        ComponentLight* light;
        ComponentSerializable* serializable;

        static EntityVisitorsView GetVisitors();
        static void OnCreate(EntityDatabase* entityDb, EntityLight& entity, const Descriptor& descriptor);
    };
}
