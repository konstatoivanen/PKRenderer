#include "PrecompiledHeader.h"
#include "App/ECS/EntityLight.h"

namespace PK::App
{
    void EntityLight::OnCreate(
        [[maybe_unused]] EntityDatabase* entityDb, 
        [[maybe_unused]] const EGID& egid, 
        const EntityLight& desc, 
        TImplementers& implementers)
    {
        // Light radius based on phyiscal attenuation at minAtten cutoff.
        const auto minAtten = 0.2f;
        const auto intensity = math::cmax(desc.color);
        const auto radius = desc.radius < 0.0f ? (intensity * intensity) / (minAtten * minAtten) : desc.radius;
        
        auto implementer = Sequence::GetAt<0>(implementers);
        implementer->localAABB = math::centerExtentsToAABB(PK_FLOAT3_ZERO, PK_FLOAT3_ONE);
        implementer->position = desc.position;
        implementer->rotation = quaternion(desc.rotation);
        implementer->scale = PK_FLOAT3_ONE;
        implementer->color = desc.color;
        implementer->radius = radius;
        implementer->sourceRadius = desc.sourceRadius;
        implementer->angle = desc.angle;
        implementer->type = desc.type;
        implementer->IESProfile = desc.IESProfile;
        implementer->useIESCandelas = desc.useIESCandelas;
        implementer->flags = App::ScenePrimitiveFlags::Light;
        implementer->flags = desc.castShadow ? implementer->flags | App::ScenePrimitiveFlags::CastShadows : implementer->flags;
        implementer->flags = desc.type == App::LightType::Directional ? implementer->flags | App::ScenePrimitiveFlags::NeverCull : implementer->flags;

        if (desc.type == App::LightType::Point)
        {
            implementer->localAABB = math::centerExtentsToAABB(PK_FLOAT3_ZERO, PK_FLOAT3_ONE * radius);
        }

        if (desc.type == App::LightType::Spot)
        {
            auto halftan = radius * math::tan(desc.angle * 0.5f * PK_FLOAT_DEG2RAD);
            implementer->localAABB = math::centerExtentsToAABB(float3(0.0f, 0.0f, radius * 0.5f), float3(halftan, halftan, radius * 0.5f));
        }
    }
}
