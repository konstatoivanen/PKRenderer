#pragma once
#include "Core/ECS/EntityFactory.h"
#include "App/Renderer/EntityEnums.h"

namespace PK::App
{
    struct EntityLightSphere
    {
        using TImplementers = Tuple<>;
        using TViews = Tuple<>;

        //static void OnDeserialize(EntityDatabase* entityDb, const EGID& egid, SerialNodeRead node, TImplementers& implementers) = delete;
        //static void OnSerialize(EntityDatabase* entityDb, const EGID& egid, SerialNodeWrite node) = delete;
        static void OnCreate(EntityDatabase* entityDb, const EGID& egid, const EntityLightSphere& descriptor, TImplementers& implementers);

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
}
