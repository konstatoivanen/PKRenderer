#pragma once
#include "Core/ECS/EntityFactory.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "App/ECS/EntityViewLight.h"

namespace PK::App
{
    struct EntityLight
    {
        struct Descriptor
        {
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

        static void OnCreate(EntityDatabase* entityDb, EntityLight& entity, const Descriptor& descriptor);
    };
}
