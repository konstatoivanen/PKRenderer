#include "PrecompiledHeader.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/IESProfile.h"
#include "App/Renderer/HashCache.h"
#include "Core/ECS/EntitySerializer.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityMeshStatic.h"
#include "App/ECS/EntityLight.h"
#include "App/ECS/EntityLightSphere.h"

namespace PK::App
{
    //EntityVisitorsView EntityLightSphere::GetVisitors()
    //{
    //    return { nullptr, 0u };
    //}

    void EntityLightSphere::OnCreate(EntityDatabase* entityDb, EntityLightSphere& entity, const Descriptor& desc)
    {
        EntityLight::Descriptor descLight;
        descLight.serialFlags = EntitySerialFlags::None;
        descLight.IESProfile = desc.IESProfile;
        descLight.position = desc.position;
        descLight.rotation = desc.rotation;
        descLight.color = desc.color;
        descLight.angle = desc.angle;
        descLight.radius = desc.radius;
        descLight.sourceRadius = desc.sourceRadius;
        descLight.type = desc.type;
        descLight.useIESCandelas = desc.useIESCandelas;
        descLight.castShadow = desc.castShadow;
        auto lightEntity = entityDb->New<EntityLight>(descLight);

        auto mesh = desc.assetDatabase->Find<MeshStatic>("Primitive_Sphere");
        auto shader = desc.assetDatabase->Find<ShaderAsset>("MS_Mat_Unlit_Color");
        MaterialTarget material{ desc.assetDatabase->CreateVirtual<Material>(FixedString32("M_Point_Light_%u", *lightEntity.entityId).c_str(), shader.get(), nullptr), 0u };
        material.material->Set<float4>(HashCache::Get()->_Color, desc.color);
        material.material->Set<float4>(HashCache::Get()->_ColorVoxelize, PK_COLOR_BLACK);

        EntityMeshStatic::Descriptor meshDesc;
        meshDesc.serialFlags = EntitySerialFlags::None;
        meshDesc.flags = ScenePrimitiveFlags::None;
        meshDesc.mesh = mesh;
        meshDesc.materials = { &material, 1u };
        meshDesc.position = desc.position;
        meshDesc.rotation = PK_FLOAT3_ZERO;
        meshDesc.scale = PK_FLOAT3_ONE * desc.sourceRadius;
        auto meshEntity = entityDb->New<EntityMeshStatic>(meshDesc);

        entity.lightSphere->lightEntityId = *lightEntity.entityId;
        entity.lightSphere->meshEntityId = *meshEntity.entityId;
    }
}
