#include "PrecompiledHeader.h"
#include "Math/FunctionsIntersect.h"
#include "ECS/Implementers/StaticMeshRenderableImplementer.h"
#include "ECS/Implementers/LightImplementer.h"
#include "ECS/EntityViews/LightSphereView.h"
#include "Rendering/HashCache.h"
#include "Builders.h"

namespace PK::ECS::Builders
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core::Services;
    using namespace PK::ECS::EntityViews;
    using namespace PK::ECS::Implementers;
    using namespace PK::Rendering;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::Structs;
    using namespace PK::Rendering::RHI::Objects;

    static BoundingBox GetSubmeshRangeBounds(StaticMesh* mesh, const std::initializer_list<MaterialTarget>& materials)
    {
        auto bounds = BoundingBox::GetMinBounds();

        for (auto& target : materials)
        {
            auto sm = mesh->GetSubmesh(target.submesh);
            Math::Functions::BoundsEncapsulate(&bounds, sm->bounds);
        }

        return bounds;
    }

    static BoundingBox GetSubmeshRangeBounds(VirtualStaticMesh* mesh, const std::initializer_list<MaterialTarget>& materials)
    {
        return GetSubmeshRangeBounds(mesh->GetStaticMesh(), materials);
    }

    EGID BuildStaticMeshRenderableEntity(EntityDatabase* entityDb,
        StaticMesh* mesh,
        const std::initializer_list<MaterialTarget>& materials,
        const float3& position,
        const float3& rotation,
        float size,
        RenderableFlags flags)
    {
        auto egid = entityDb->ReserveEntityId((uint)ENTITY_GROUPS::ACTIVE);
        auto implementer = entityDb->ReserveImplementer<StaticMeshRenderableImplementer>();
        BuildTransformView(entityDb, implementer, egid, position, rotation, PK_FLOAT3_ONE * size, GetSubmeshRangeBounds(mesh, materials));
        BuildBaseRenderableView(entityDb, implementer, egid, flags | RenderableFlags::Mesh);
        BuildStaticMeshRenderableView(entityDb, implementer, egid, mesh, materials);
        return egid;
    }

    EGID BuildStaticMeshRenderableEntity(EntityDatabase* entityDb,
        VirtualStaticMesh* mesh,
        const std::initializer_list<MaterialTarget>& materials,
        const float3& position,
        const float3& rotation,
        float size,
        RenderableFlags flags)
    {
        auto egid = entityDb->ReserveEntityId((uint)ENTITY_GROUPS::ACTIVE);
        auto implementer = entityDb->ReserveImplementer<StaticMeshRenderableImplementer>();
        BuildTransformView(entityDb, implementer, egid, position, rotation, PK_FLOAT3_ONE * size, GetSubmeshRangeBounds(mesh, materials));
        BuildBaseRenderableView(entityDb, implementer, egid, flags | RenderableFlags::Mesh);
        BuildStaticMeshRenderableView(entityDb, implementer, egid, mesh->GetStaticMesh(), materials);
        return egid;
    }

    EGID BuildLightRenderableEntity(EntityDatabase* entityDb,
        AssetDatabase* assetDatabase,
        const float3& position,
        const float3& rotation,
        LightType type,
        Cookie cookie,
        const color& color,
        float angle,
        float radius,
        bool castShadows)
    {
        auto egid = entityDb->ReserveEntityId((uint)ENTITY_GROUPS::ACTIVE);
        auto implementer = entityDb->ReserveImplementer<LightImplementer>();
        BuildTransformView(entityDb, implementer, egid, position, rotation, PK_FLOAT3_ONE, {});
        BuildLightRenderableViews(entityDb, implementer, egid, type, cookie, color, angle, radius, castShadows);
        auto lightSphereView = entityDb->ReserveEntityView<LightSphereView>(egid);
        return egid;
    }

    EGID BuildLightSphereRenderableEntity(EntityDatabase* entityDb,
        AssetDatabase* assetDatabase,
        const float3& position,
        LightType type,
        Cookie cookie,
        const color& color,
        bool castShadows)
    {
        const float kLightSourceRadius = 0.2f;

        auto egid = entityDb->ReserveEntityId((uint)ENTITY_GROUPS::ACTIVE);
        auto implementer = entityDb->ReserveImplementer<LightImplementer>();
        BuildTransformView(entityDb, implementer, egid, position, PK_FLOAT3_ZERO, PK_FLOAT3_ONE, {});
        BuildLightRenderableViews(entityDb, implementer, egid, type, cookie, color, 90.0f, -1.0f, castShadows);
        auto lightSphereView = entityDb->ReserveEntityView<LightSphereView>(egid);

        implementer->sourceRadius = kLightSourceRadius;

        auto mesh = assetDatabase->Find<VirtualStaticMesh>("Primitive_Sphere");
        auto shader = assetDatabase->Find<Shader>("MS_Mat_Unlit_Color");
        auto material = assetDatabase->RegisterProcedural("M_Point_Light_" + std::to_string(egid.entityID()), CreateRef<Material>(shader, nullptr));
        material->Set<float4>(HashCache::Get()->_Color, color);
        material->Set<float4>(HashCache::Get()->_ColorVoxelize, PK_COLOR_BLACK);

        auto meshEgid = BuildStaticMeshRenderableEntity(entityDb, mesh, { { material, 0 } }, position, PK_FLOAT3_ZERO, implementer->sourceRadius, RenderableFlags::Cullable);
        lightSphereView->transformMesh = entityDb->Query<EntityViews::TransformView>(meshEgid)->transform;
        lightSphereView->transformLight = implementer;
        return egid;
    }
}