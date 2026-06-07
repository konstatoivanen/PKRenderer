#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/ShaderAsset.h"
#include "App/Renderer/HashCache.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewLightSphereTransforms.h"
#include "App/ECS/EntityMeshStatic.h"
#include "App/ECS/EntityLight.h"
#include "App/ECS/EntityLightSphere.h"

namespace PK
{
    template<>
    EGID EntityFactory<App::EntityLightSphere>::Create(EntityDatabase* entityDb, EGID egid, const App::EntityLightSphere& desc)
    {
        App::EntityLight descLight;
        descLight.type = desc.type;
        descLight.iesProfile = desc.iesProfile;
        descLight.position = desc.position;
        descLight.rotation = desc.rotation;
        descLight.color = desc.color;
        descLight.angle = desc.angle;
        descLight.radius = desc.radius;
        descLight.sourceRadius = desc.sourceRadius;
        descLight.castShadow = desc.castShadow;
        auto lightEgid = EntityFactory<App::EntityLight>::Create(entityDb, egid, descLight);

        auto mesh = desc.assetDatabase->Find<MeshStatic>("Primitive_Sphere");
        auto shader = desc.assetDatabase->Find<ShaderAsset>("MS_Mat_Unlit_Color");
        MaterialTarget material{ desc.assetDatabase->CreateVirtual<Material>(FixedString32("M_Point_Light_%u", lightEgid.entityID()).c_str(), shader.get(), nullptr), 0u };
        material.material->Set<float4>(App::HashCache::Get()->_Color, desc.color);
        material.material->Set<float4>(App::HashCache::Get()->_ColorVoxelize, PK_COLOR_BLACK);

        App::EntityMeshStatic meshDesc;
        meshDesc.flags = App::ScenePrimitiveFlags::None;
        meshDesc.mesh = mesh;
        meshDesc.materials = { &material, 1u };
        meshDesc.position = desc.position;
        meshDesc.rotation = PK_FLOAT3_ZERO;
        meshDesc.scale = PK_FLOAT3_ONE * desc.sourceRadius;
        auto meshEgid = EntityFactory<App::EntityMeshStatic>::Create(entityDb, EGID(0u, egid.groupID()), meshDesc);

        auto lightSphereView = entityDb->NewView<App::EntityViewLightSphereTransforms>(lightEgid);
        lightSphereView->transformMesh = entityDb->Query<App::EntityViewTransform>(meshEgid)->transform;
        lightSphereView->transformLight = entityDb->Query<App::EntityViewTransform>(lightEgid)->transform;

        return lightEgid;
    }

    template<>
    EGID EntityFactory<App::EntityLightSphere>::CreateDefault(EntityDatabase* entityDb, EGID egid)
    {
        return EGIDInvalid;
    }

    template<>
    EGID EntityFactory<App::EntityLightSphere>::Deserialize(EntityDatabase* entityDb, const YAML::ConstNode& parent, uint32_t group)
    {
        return EGIDInvalid;
    }

    template<>
    void EntityFactory<App::EntityLightSphere>::Serialize(EntityDatabase* entityDb, YAML::Node& parent, const EGID& egid)
    {

    }
}
