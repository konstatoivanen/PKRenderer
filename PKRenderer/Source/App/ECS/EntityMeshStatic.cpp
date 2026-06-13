#include "PrecompiledHeader.h"
#include "Core/Yaml/RapidyamlPrivate.h"
#include "Core/Math/Math.h"
#include "Core/CLI/Log.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Rendering/Material.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/Mesh.h"
#include "App/Renderer/EntityEnums.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "App/ECS/EntityViewMeshStatic.h"
#include "App/ECS/EntityMeshStatic.h"

namespace PK
{
    template<> 
    EGID EntityFactory<App::EntityMeshStatic>::Create(EntityDatabase* entityDb, EGID egid, const App::EntityMeshStatic& desc)
    {
        if (egid.entityID() == 0u)
        {
            egid = entityDb->ReserveEntityId(egid.groupID());
        }

        auto implementer = entityDb->NewImplementer<App::ImplementerMeshStatic>();
        entityDb->NewView<App::EntityViewTransform>(implementer, egid);
        entityDb->NewView<App::EntityViewScenePrimitive>(implementer, egid);
        entityDb->NewView<App::EntityViewMeshStatic>(implementer, egid);

        implementer->position = desc.position;
        implementer->rotation = quaternion(desc.rotation);
        implementer->scale = desc.scale;
        implementer->flags = desc.flags | App::ScenePrimitiveFlags::Mesh;
        implementer->materials.Copy(desc.materials.data, desc.materials.count);
        implementer->sharedMesh = desc.mesh;
        implementer->localAABB = PK_FLOAT3_MIN_AABB;

        for (auto& target : implementer->materials)
        {
            implementer->localAABB |= desc.mesh->GetSubmesh(target.submesh).bounds;
        }

        return egid;
    }
    
    template<>
    EGID EntityFactory<App::EntityMeshStatic>::CreateDefault(EntityDatabase* entityDb, EGID egid)
    {
        return EGIDInvalid;
    }

    template<>
    EGID EntityFactory<App::EntityMeshStatic>::Deserialize(EntityDatabase* entityDb, const YAML::ConstNode& parent, uint32_t group)
    {
        App::EntityMeshStatic descriptor;
        YAML::Read<float3>(parent, "position", &descriptor.position);
        YAML::Read<float3>(parent, "rotation", &descriptor.rotation);
        YAML::Read<float3>(parent, "scale", &descriptor.scale);
        YAML::Read<uint8_t>(parent, "flags", reinterpret_cast<uint8_t*>(&descriptor.flags));
        YAML::Read<MeshStaticRef>(parent, "mesh", &descriptor.mesh);
        
        auto materials = parent.find_child("materials");
        auto materialCount = materials.num_children();
        auto materialArray = PK_STACK_ALLOC(MaterialTarget, materialCount);

        for (auto i = 0u; i < materialCount; ++i)
        {
            YAML::Read<MaterialTarget>(materials[i], materialArray + i);
        }

        descriptor.materials = { materialArray, materialCount };

        return Create(entityDb, group, descriptor);
    }

    template<>
    void EntityFactory<App::EntityMeshStatic>::Serialize(EntityDatabase* entityDb, YAML::Node& parent, const EGID& egid)
    {
        auto viewTransform = entityDb->Query<App::EntityViewTransform>(egid);
        auto viewMeshStatic = entityDb->Query<App::EntityViewMeshStatic>(egid);
        auto rotationEuler = math::euler(viewTransform->transform->rotation);

        YAML::Write<float3>(parent, "position", &viewTransform->transform->position);
        YAML::Write<float3>(parent, "rotation", &rotationEuler);
        YAML::Write<float3>(parent, "scale", &viewTransform->transform->scale);
        YAML::Write<uint8_t>(parent, "flags", reinterpret_cast<const uint8_t*>(&viewMeshStatic->primitive->flags));
        YAML::Write<MeshStaticRef>(parent, "mesh", &viewMeshStatic->staticMesh->sharedMesh);

        auto materials = parent["materials"];
        materials |= ryml::SEQ;

        for (auto i = 0u; i < viewMeshStatic->materials->materials.GetCount(); ++i)
        {
            auto node = materials.append_child();
            YAML::Write<MaterialTarget>(node, nullptr, &viewMeshStatic->materials->materials[i]);
        }
    }
}
