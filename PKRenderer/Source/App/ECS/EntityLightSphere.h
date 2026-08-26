#pragma once
#include "Core/ECS/EntityComponentSerializable.h"
#include "App/Renderer/EntityEnums.h"

namespace PK
{
    struct EntityDatabase;
    struct EntityVisitorsView;
}

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
            FixedString64 entityName;
            EntitySerialFlags serialFlags;

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
        ComponentSerializable* serializable;

        static EntityVisitorsView GetVisitors();
        static void OnCreate(EntityDatabase* entityDb, EntityLightSphere& entity, const Descriptor& descriptor);
        static void OnSerialize(EntityDatabase* entityDb, EntityLightSphere& entity, SerialNodeWrite& node);
        static void OnDeserialize(EntityDatabase* entityDb, EntityLightSphere& entity, SerialNodeRead& node);
    };
}
