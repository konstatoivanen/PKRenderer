#pragma once
#include "Core/Math/Math.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/ECS/EntityFactory.h"
#include "App/Renderer/EntityEnums.h"

namespace PK::App
{
    struct EntityLightSphere : EntityFactory<EntityLightSphere>
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
}
