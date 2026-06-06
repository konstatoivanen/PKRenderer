#include "PrecompiledHeader.h"
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
        return EGIDInvalid;
    }

    template<>
    void EntityFactory<App::EntityMeshStatic>::Serialize(EntityDatabase* entityDb, YAML::Node& parent, const EGID& egid)
    {

    }
}
