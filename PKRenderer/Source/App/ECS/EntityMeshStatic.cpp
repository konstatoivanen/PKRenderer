#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Math/Math.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Rendering/Material.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/Mesh.h"
#include "Core/Serialization/Serialize.h"
#include "App/Renderer/EntityEnums.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "App/ECS/EntityViewMeshStatic.h"
#include "App/ECS/EntityMeshStatic.h"

namespace PK::App
{
    void EntityMeshStatic::OnCreate(
        [[maybe_unused]] EntityDatabase* entityDb, 
        [[maybe_unused]] const EGID& egid, 
        const EntityMeshStatic& desc, 
        TImplementers& implementers)
    {
        auto implementer = Sequence::GetAt<0>(implementers);
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
    }
    
    /*
    template<>
    EGID EntityFactory<App::EntityMeshStatic>::Deserialize(EntityDatabase* entityDb, SerialNodeRead parent, uint32_t group)
    {
        App::EntityMeshStatic descriptor;
        Serialize::ReadVal<float3>(parent, "position", &descriptor.position);
        Serialize::ReadVal<float3>(parent, "rotation", &descriptor.rotation);
        Serialize::ReadVal<float3>(parent, "scale", &descriptor.scale);
        Serialize::ReadVal<uint8_t>(parent, "flags", reinterpret_cast<uint8_t*>(&descriptor.flags));
        Serialize::ReadVal<MeshStaticRef>(parent, "mesh", &descriptor.mesh);
        
        auto materials = parent.find_child("materials");
        auto materialCount = materials.num_children();
        auto materialArray = PK_STACK_ALLOC(MaterialTarget, materialCount);

        for (auto i = 0u; i < materialCount; ++i)
        {
            Serialize::ReadVal<MaterialTarget>(materials[i], materialArray + i);
        }

        descriptor.materials = { materialArray, materialCount };

        return Create(entityDb, group, descriptor);
    }

    template<>
    void EntityFactory<App::EntityMeshStatic>::Serialize(EntityDatabase* entityDb, SerialNodeWrite parent, const EGID& egid)
    {
        auto viewTransform = entityDb->Query<App::EntityViewTransform>(egid);
        auto viewMeshStatic = entityDb->Query<App::EntityViewMeshStatic>(egid);
        auto rotationEuler = math::euler(viewTransform->transform->rotation);

        Serialize::WriteVal<float3>(parent, "position", &viewTransform->transform->position);
        Serialize::WriteVal<float3>(parent, "rotation", &rotationEuler);
        Serialize::WriteVal<float3>(parent, "scale", &viewTransform->transform->scale);
        Serialize::WriteVal<uint8_t>(parent, "flags", reinterpret_cast<const uint8_t*>(&viewMeshStatic->primitive->flags));
        Serialize::WriteVal<MeshStaticRef>(parent, "mesh", &viewMeshStatic->staticMesh->sharedMesh);

        auto materials = parent["materials"];
        materials |= ryml::SEQ;

        for (auto i = 0u; i < viewMeshStatic->materials->materials.GetCount(); ++i)
        {
            auto node = materials.append_child();
            Serialize::WriteVal<MaterialTarget>(node, nullptr, &viewMeshStatic->materials->materials[i]);
        }
    }
    */
}
