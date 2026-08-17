#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/ShaderAsset.h"
#include "App/Renderer/HashCache.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityMeshStatic.h"
#include "App/ECS/EntityLight.h"
#include "App/ECS/EntityLightSphere.h"

namespace PK::App
{
    void EntityLightSphere::OnCreate(EntityDatabase* entityDb, EntityLightSphere& entity, const Descriptor& desc)
    {
        EntityLight::Descriptor descLight;
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
        auto lightEntity = EntityFactory<EntityLight>::Create(entityDb, descLight);

        auto mesh = desc.assetDatabase->Find<MeshStatic>("Primitive_Sphere");
        auto shader = desc.assetDatabase->Find<ShaderAsset>("MS_Mat_Unlit_Color");
        MaterialTarget material{ desc.assetDatabase->CreateVirtual<Material>(FixedString32("M_Point_Light_%u", *lightEntity.entityId).c_str(), shader.get(), nullptr), 0u };
        material.material->Set<float4>(HashCache::Get()->_Color, desc.color);
        material.material->Set<float4>(HashCache::Get()->_ColorVoxelize, PK_COLOR_BLACK);

        EntityMeshStatic::Descriptor meshDesc;
        meshDesc.entitySerialize = false;
        meshDesc.flags = ScenePrimitiveFlags::None;
        meshDesc.mesh = mesh;
        meshDesc.materials = { &material, 1u };
        meshDesc.position = desc.position;
        meshDesc.rotation = PK_FLOAT3_ZERO;
        meshDesc.scale = PK_FLOAT3_ONE * desc.sourceRadius;
        auto meshEntity = EntityFactory<EntityMeshStatic>::Create(entityDb, meshDesc);

        entity.lightSphere->lightEntityId = *lightEntity.entityId;
        entity.lightSphere->meshEntityId = *meshEntity.entityId;
    }
}
