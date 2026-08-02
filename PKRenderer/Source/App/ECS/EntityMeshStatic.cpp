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
}
