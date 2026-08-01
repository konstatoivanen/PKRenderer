#pragma once
#include "Core/ECS/IEntityImplementer.h"
#include "Core/ECS/EntityFactory.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "App/ECS/EntityViewLight.h"

namespace PK::App
{
    struct ImplementerLight : public IEntityImplementer,
        public ComponentTransform,
        public ComponentBounds,
        public ComponentScenePrimitive,
        public ComponentLight
    {
    };

    struct EntityLight
    {
        using TImplementers = Tuple<ImplementerLight*>;
        using TViews = Tuple<EntityViewTransform, EntityViewScenePrimitive, EntityViewLight>;
        constexpr static const bool IsSerializable = false;

        //static void OnDeserialize(EntityDatabase* entityDb, const EGID& egid, SerialNodeRead node, TImplementers& implementers) = delete;
        //static void OnSerialize(EntityDatabase* entityDb, const EGID& egid, SerialNodeWrite node) = delete;
        static void OnCreate(EntityDatabase* entityDb, const EGID& egid, const EntityLight& descriptor, TImplementers& implementers);

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
}
