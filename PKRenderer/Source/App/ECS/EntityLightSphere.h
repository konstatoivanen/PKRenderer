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
        LightType type;
        IESProfileRef iesProfile;
        float3 position;
        float3 rotation;
        color color;
        float angle;
        float radius;
        float sourceRadius;
        bool castShadow;
    };
}
