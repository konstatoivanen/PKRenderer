#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "App/ECS/EntityLight.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "App/ECS/EntityViewLight.h"

namespace PK
{
    template<>
    EGID EntityFactory<App::EntityLight>::Create(EntityDatabase* entityDb, EGID egid, const App::EntityLight& desc)
    {
        // Light radius based on phyiscal attenuation at minAtten cutoff.
        const auto minAtten = 0.2f;
        const auto intensity = math::cmax(desc.color);
        const auto radius = desc.radius < 0.0f ? (intensity * intensity) / (minAtten * minAtten) : desc.radius;
        
        if (egid.entityID() == 0u)
        {
            egid = entityDb->ReserveEntityId(egid.groupID());
        }

        auto implementer = entityDb->NewImplementer<App::ImplementerLight>();

        entityDb->NewView<App::EntityViewTransform>(implementer, egid);
        entityDb->NewView<App::EntityViewScenePrimitive>(implementer, egid);
        entityDb->NewView<App::EntityViewLight>(implementer, egid);
        implementer->localAABB = math::centerExtentsToAABB(PK_FLOAT3_ZERO, PK_FLOAT3_ONE);
        implementer->position = desc.position;
        implementer->rotation = quaternion(desc.rotation);
        implementer->scale = PK_FLOAT3_ONE;
        implementer->color = desc.color;
        implementer->radius = radius;
        implementer->sourceRadius = desc.sourceRadius;
        implementer->angle = desc.angle;
        implementer->cookie = desc.cookie;
        implementer->type = desc.type;
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

        return egid;
    }

    template<>
    EGID EntityFactory<App::EntityLight>::CreateDefault(EntityDatabase* entityDb, EGID egid)
    {
        return EGIDInvalid;
    }

    template<>
    EGID EntityFactory<App::EntityLight>::Deserialize(EntityDatabase* entityDb, const YAML::ConstNode& parent, uint32_t group)
    {
        return EGIDInvalid;
    }

    template<>
    void EntityFactory<App::EntityLight>::Serialize(EntityDatabase* entityDb, YAML::Node& parent, const EGID& egid)
    {

    }
}
