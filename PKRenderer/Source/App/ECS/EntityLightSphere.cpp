#include "PrecompiledHeader.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/IESProfile.h"
#include "App/Renderer/HashCache.h"
#include "Core/ECS/EntityArchive.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityMeshStatic.h"
#include "App/ECS/EntityLight.h"
#include "App/ECS/EntityLightSphere.h"

namespace PK::App
{
    EntityVisitorsView EntityLightSphere::GetVisitors()
    {
        return MakeEntityVisitorsView<
            EntityArchive::Serialize<EntityLightSphere>,
            EntityArchive::Deserialize<EntityLightSphere>>();
    }

    void EntityLightSphere::OnCreate(EntityDatabase* entityDb, EntityLightSphere& entity, const Descriptor& desc)
    {
        EntityLight::Descriptor descLight;
        descLight.sceneId = 0u;
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
        meshDesc.sceneId = 0u;
        meshDesc.flags = ScenePrimitiveFlags::None;
        meshDesc.mesh = mesh;
        meshDesc.materials = { &material, 1u };
        meshDesc.position = desc.position;
        meshDesc.rotation = PK_FLOAT3_ZERO;
        meshDesc.scale = PK_FLOAT3_ONE * desc.sourceRadius;
        auto meshEntity = entityDb->New<EntityMeshStatic>(meshDesc);

        entity.serializable->name = desc.entityName;
        entity.serializable->sceneId = desc.sceneId;
        entity.lightSphere->lightEntityId = *lightEntity.entityId;
        entity.lightSphere->meshEntityId = *meshEntity.entityId;
    }
    
    void EntityLightSphere::OnSerialize(EntityDatabase* entityDb, EntityLightSphere& entity, EntityArchiveWrite* archive)
    {
        auto& node = *archive->node;
        auto light = entityDb->Query<EntityLight>(entity.lightSphere->lightEntityId);
        auto euler = math::euler(light.transform->rotation);
        auto castShadows = (light.primitive->flags & ScenePrimitiveFlags::CastShadows) != 0u;
        Serialize::WriteSingle<IESProfileRef>(node["IESProfile"], &light.light->IESProfile);
        Serialize::WriteSingle<float3>(node["position"], &light.transform->position);
        Serialize::WriteSingle<float3>(node["rotation"], &euler);
        Serialize::WriteSingle<color>(node["color"],  &light.light->color);
        Serialize::WriteSingle<float>(node["angle"], &light.light->angle);
        Serialize::WriteSingle<float>(node["radius"], &light.light->radius);
        Serialize::WriteSingle<float>(node["sourceRadius"], &light.light->sourceRadius);
        Serialize::WriteSingle<LightType>(node["lighttype"], &light.light->type);
        Serialize::WriteSingle<bool>(node["useIESCandelas"], &light.light->useIESCandelas);
        Serialize::WriteSingle<bool>(node["castShadow"], &castShadows);
    }

    void EntityLightSphere::OnDeserialize(EntityDatabase* entityDb, EntityLightSphere& entity, EntityArchiveRead* archive)
    {
        Descriptor descriptor;
        auto& node = *archive->node;
        descriptor.assetDatabase = AssetDatabase::Get();
        descriptor.entityName = entity.serializable->name;
        descriptor.sceneId = entity.serializable->sceneId;
        Serialize::ReadSingle<IESProfileRef>(node["IESProfile"], &descriptor.IESProfile);
        Serialize::ReadSingle<float3>(node["position"], &descriptor.position);
        Serialize::ReadSingle<float3>(node["rotation"], &descriptor.rotation);
        Serialize::ReadSingle<color>(node["color"], &descriptor.color);
        Serialize::ReadSingle<float>(node["angle"], &descriptor.angle);
        Serialize::ReadSingle<float>(node["radius"], &descriptor.radius);
        Serialize::ReadSingle<float>(node["sourceRadius"], &descriptor.sourceRadius);
        Serialize::ReadSingle<LightType>(node["lighttype"], &descriptor.type);
        Serialize::ReadSingle<bool>(node["useIESCandelas"], &descriptor.useIESCandelas);
        Serialize::ReadSingle<bool>(node["castShadow"], &descriptor.castShadow);
        OnCreate(entityDb, entity, descriptor);
    }
}
