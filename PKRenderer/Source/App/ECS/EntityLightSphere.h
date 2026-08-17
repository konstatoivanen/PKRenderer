#pragma once
#include "Core/ECS/EntityFactory.h"
#include "App/Renderer/EntityEnums.h"

namespace PK::App
{
    struct ComponentLightSphere
    {
        uint32_t lightEntityId;
        uint32_t meshEntityId;
    };

    struct EntityLightSphere
    {
        struct Descriptor
        {
            AssetDatabase* assetDatabase;
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
        ComponentLightSphere* lightSphere;

        static void OnCreate(EntityDatabase* entityDb, EntityLightSphere& entity, const Descriptor& descriptor);
    };
}
