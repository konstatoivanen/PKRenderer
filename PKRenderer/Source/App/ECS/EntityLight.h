#pragma once
#include "Core/ECS/IEntityImplementer.h"
#include "Core/ECS/EntityFactory.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentBounds.h"
#include "App/ECS/ComponentScenePrimitive.h"
#include "App/ECS/ComponentLight.h"

namespace PK::App
{
    struct ImplementerLight : public IEntityImplementer,
        public ComponentTransform,
        public ComponentBounds,
        public ComponentScenePrimitive,
        public ComponentLight
    {
    };

    struct EntityLight : EntityFactory<EntityLight>
    {
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
