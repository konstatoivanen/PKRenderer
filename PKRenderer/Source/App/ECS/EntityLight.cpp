#include "PrecompiledHeader.h"
#include "App/ECS/EntityLight.h"

namespace PK::App
{
    void EntityLight::OnCreate([[maybe_unused]] EntityDatabase* entityDb, EntityLight& entity, const Descriptor& desc)
    {
        // Light radius based on phyiscal attenuation at minAtten cutoff.
        const auto minAtten = 0.2f;
        const auto intensity = math::cmax(desc.color);
        const auto radius = desc.radius < 0.0f ? (intensity * intensity) / (minAtten * minAtten) : desc.radius;
        
        entity.bounds->localAABB = math::centerExtentsToAABB(PK_FLOAT3_ZERO, PK_FLOAT3_ONE);
        entity.transform->position = desc.position;
        entity.transform->rotation = quaternion(desc.rotation);
        entity.transform->scale = PK_FLOAT3_ONE;
        entity.light->color = desc.color;
        entity.light->radius = radius;
        entity.light->sourceRadius = desc.sourceRadius;
        entity.light->angle = desc.angle;
        entity.light->type = desc.type;
        entity.light->IESProfile = desc.IESProfile;
        entity.light->useIESCandelas = desc.useIESCandelas;

        auto flags = ScenePrimitiveFlags::Light;
        flags = desc.castShadow ? flags | ScenePrimitiveFlags::CastShadows : flags;
        flags = desc.type == App::LightType::Directional ? flags | ScenePrimitiveFlags::NeverCull : flags;
        entity.primitive->flags = flags;

        if (desc.type == App::LightType::Point)
        {
            entity.bounds->localAABB = math::centerExtentsToAABB(PK_FLOAT3_ZERO, PK_FLOAT3_ONE * radius);
        }

        if (desc.type == App::LightType::Spot)
        {
            auto halftan = radius * math::tan(desc.angle * 0.5f * PK_FLOAT_DEG2RAD);
            entity.bounds->localAABB = math::centerExtentsToAABB(float3(0.0f, 0.0f, radius * 0.5f), float3(halftan, halftan, radius * 0.5f));
        }
    }
}
