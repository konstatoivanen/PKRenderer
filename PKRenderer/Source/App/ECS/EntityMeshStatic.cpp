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
    void EntityMeshStatic::OnCreate([[maybe_unused]] EntityDatabase* entityDb, EntityMeshStatic& entity, const Descriptor& desc)
    {
        entity.transform->position = desc.position;
        entity.transform->rotation = quaternion(desc.rotation);
        entity.transform->scale = desc.scale;
        entity.primitive->flags = desc.flags | ScenePrimitiveFlags::Mesh;
        entity.materials->materials.Copy(desc.materials.data, desc.materials.count);
        entity.mesh->sharedMesh = desc.mesh;
        entity.bounds->localAABB = PK_FLOAT3_MIN_AABB;

        for (auto& target : entity.materials->materials)
        {
            entity.bounds->localAABB |= desc.mesh->GetSubmesh(target.submesh).bounds;
        }
    }
}
