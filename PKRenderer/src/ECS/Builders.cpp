#include "PrecompiledHeader.h"
#include "Math/FunctionsIntersect.h"
#include "Core/Assets/AssetDatabase.h"
#include "ECS/ImplementerStaticMesh.h"
#include "ECS/ImplementerFlyCamera.h"
#include "ECS/ImplementerLight.h"
#include "ECS/EntityViewLightSphereTransforms.h"
#include "ECS/EntityViewRenderView.h"
#include "ECS/EntityViewFlyCamera.h"
#include "ECS/EntityViewTransform.h"
#include "ECS/EntityViewScenePrimitive.h"
#include "ECS/EntityViewStaticMesh.h"
#include "ECS/EntityViewLight.h"
#include "ECS/EntityViewFlyCamera.h"
#include "Rendering/RHI/Objects/Shader.h"
#include "Rendering/Objects/VirtualStaticMesh.h"
#include "Rendering/HashCache.h"
#include "Builders.h"

namespace PK::ECS::Build
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::ECS;
    using namespace PK::Rendering;
    using namespace PK::Rendering::Objects;
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

    template<typename T>
    static void EntityViewTransform(EntityDatabase* entityDb,
        T* implementer,
        const EGID& egid,
        const Math::float3& position,
        const Math::float3& rotation,
        const Math::float3& scale,
        const Math::BoundingBox& localBounds)
    {
        entityDb->ReserveView(implementer, egid, &EntityViewTransform::bounds, &EntityViewTransform::transform);
        implementer->localAABB = localBounds;
        implementer->position = position;
        implementer->rotation = glm::quat(rotation);
        implementer->scale = scale;
    }

    template<typename T>
    static void EntityViewScenePrimitive(EntityDatabase* entityDb,
        T* implementer,
        const EGID& egid,
        Rendering::ScenePrimitiveFlags flags)
    {
        entityDb->ReserveView(implementer, egid, &EntityViewScenePrimitive::primitive, &EntityViewScenePrimitive::bounds);
        implementer->flags = flags;
    }

    template<typename T>
    static void EntityViewStaticMesh(EntityDatabase* entityDb,
        T* implementer,
        const EGID& egid,
        Rendering::Objects::StaticMesh* staticMesh,
        const std::initializer_list<Rendering::Objects::MaterialTarget>& materials)
    {
        entityDb->ReserveView(implementer, egid, &EntityViewStaticMesh::primitive, &EntityViewStaticMesh::materials, &EntityViewStaticMesh::staticMesh, &EntityViewStaticMesh::transform);
        implementer->materials = materials;
        implementer->sharedMesh = staticMesh;
    }

    template<typename T>
    static void EntityViewLight(EntityDatabase* entityDb,
        T* implementer,
        const EGID& egid,
        Rendering::LightType type,
        Rendering::LightCookie cookie,
        const Math::color& color,
        float radius,
        float angle)
    {
        entityDb->ReserveView(implementer, egid, &EntityViewLight::transform, &EntityViewLight::bounds, &EntityViewLight::light, &EntityViewLight::primitive);
        implementer->color = color;
        implementer->radius = radius;
        implementer->angle = angle;
        implementer->cookie = cookie;
        implementer->type = type;
    }

    template<typename T>
    static void EntityViewLightPrimitive(EntityDatabase* entityDb,
        T* implementer,
        const EGID& egid,
        Rendering::LightType type,
        Rendering::LightCookie cookie,
        const Math::color& color,
        float angle,
        float radius,
        bool castShadows)
    {
        // Light radius based on phyiscal attenuation at minAtten cutoff.
        const auto minAtten = 0.2f;
        auto intensity = glm::compMax(color);
        radius = radius < 0.0f ? (intensity * intensity) / (minAtten * minAtten) : radius;

        auto flags = Rendering::ScenePrimitiveFlags::Light;

        if (castShadows)
        {
            flags = flags | Rendering::ScenePrimitiveFlags::CastShadows;
        }

        switch (type)
        {
            case LightType::Directional:
            {
                implementer->localAABB = BoundingBox::CenterExtents(PK_FLOAT3_ZERO, PK_FLOAT3_ONE);
                flags = flags | ScenePrimitiveFlags::NeverCull;
            }
            break;
            case LightType::Point:
            {
                implementer->localAABB = BoundingBox::CenterExtents(PK_FLOAT3_ZERO, PK_FLOAT3_ONE * radius);
            }
            break;
            case LightType::Spot:
            {
                auto a = radius * glm::tan(angle * 0.5f * PK_FLOAT_DEG2RAD);
                implementer->localAABB = BoundingBox::CenterExtents({ 0.0f, 0.0f, radius * 0.5f }, { a, a, radius * 0.5f });
            }
            break;
            default: PK_THROW_ERROR("Invalid Light Type");
        }

        EntityViewScenePrimitive(entityDb, implementer, egid, flags);
        EntityViewLight(entityDb, implementer, egid, type, cookie, color, radius, angle);
    }

    template<typename T>
    static void EntityViewRenderView(EntityDatabase* entityDb, T* implementer, const EGID& egid, Rendering::RenderViewType type, const Math::uint4& desiredRect, bool isWindowTarget)
    {
        entityDb->ReserveView(implementer, egid, &EntityViewRenderView::transform, &EntityViewRenderView::projection, &EntityViewRenderView::renderView);
        implementer->type = type;
        implementer->desiredRect = desiredRect;
        implementer->isWindowTarget = isWindowTarget;
    }

    template<typename T>
    static void EntityViewFlyCamera(EntityDatabase* entityDb,
        T* implementer,
        const EGID& egid,
        float moveSpeed,
        float fieldOfView,
        float zNear,
        float zFar,
        float moveSmoothing,
        float rotationSmoothing,
        float sensitivity)
    {
        entityDb->ReserveView(implementer, egid, &EntityViewFlyCamera::transform, &EntityViewFlyCamera::projection, &EntityViewFlyCamera::flyCamera);
        implementer->mode = ComponentProjection::Perspective;
        implementer->snashotPosition = implementer->position;
        implementer->snashotRotation = glm::eulerAngles(implementer->rotation);
        implementer->targetPosition = implementer->position;
        implementer->eulerAngles = implementer->snashotRotation;
        implementer->fieldOfView = fieldOfView;
        implementer->zNear = zNear;
        implementer->zFar = zFar;
        implementer->moveSpeed = moveSpeed;
        implementer->moveSmoothing = moveSmoothing;
        implementer->rotationSmoothing = rotationSmoothing;
        implementer->sensitivity = sensitivity;
    }

    EGID StaticMeshEntity(EntityDatabase* entityDb,
        StaticMesh* mesh,
        const std::initializer_list<MaterialTarget>& materials,
        const float3& position,
        const float3& rotation,
        float size,
        ScenePrimitiveFlags flags)
    {
        auto egid = entityDb->ReserveEntityId((uint)ENTITY_GROUPS::ACTIVE);
        auto implementer = entityDb->ReserveImplementer<ImplementerStaticMesh>();
        EntityViewTransform(entityDb, implementer, egid, position, rotation, PK_FLOAT3_ONE * size, GetSubmeshRangeBounds(mesh, materials));
        EntityViewScenePrimitive(entityDb, implementer, egid, flags | ScenePrimitiveFlags::Mesh);
        EntityViewStaticMesh(entityDb, implementer, egid, mesh, materials);
        return egid;
    }

    EGID LightEntity(EntityDatabase* entityDb,
        AssetDatabase* assetDatabase,
        const float3& position,
        const float3& rotation,
        LightType type,
        LightCookie cookie,
        const color& color,
        float angle,
        float radius,
        bool castShadows)
    {
        auto egid = entityDb->ReserveEntityId((uint)ENTITY_GROUPS::ACTIVE);
        auto implementer = entityDb->ReserveImplementer<ImplementerLight>();
        EntityViewTransform(entityDb, implementer, egid, position, rotation, PK_FLOAT3_ONE, {});
        EntityViewLightPrimitive(entityDb, implementer, egid, type, cookie, color, angle, radius, castShadows);
        return egid;
    }

    EGID LightSphereEntity(EntityDatabase* entityDb,
        AssetDatabase* assetDatabase,
        const float3& position,
        LightType type,
        LightCookie cookie,
        const color& color,
        bool castShadows)
    {
        const float kLightSourceRadius = 0.2f;

        auto egid = entityDb->ReserveEntityId((uint)ENTITY_GROUPS::ACTIVE);
        auto implementer = entityDb->ReserveImplementer<ImplementerLight>();
        EntityViewTransform(entityDb, implementer, egid, position, PK_FLOAT3_ZERO, PK_FLOAT3_ONE, {});
        EntityViewLightPrimitive(entityDb, implementer, egid, type, cookie, color, 90.0f, 20.0f, castShadows);
        auto lightSphereView = entityDb->ReserveView<EntityViewLightSphereTransforms>(egid);

        implementer->sourceRadius = kLightSourceRadius;

        auto mesh = assetDatabase->Find<VirtualStaticMesh>("Primitive_Sphere")->GetStaticMesh();
        auto shader = assetDatabase->Find<Shader>("MS_Mat_Unlit_Color");
        auto material = assetDatabase->Register("M_Point_Light_" + std::to_string(egid.entityID()), CreateRef<Material>(shader, nullptr));
        material->Set<float4>(HashCache::Get()->_Color, color);
        material->Set<float4>(HashCache::Get()->_ColorVoxelize, PK_COLOR_BLACK);

        auto meshEgid = StaticMeshEntity(entityDb, mesh, { { material, 0 } }, position, PK_FLOAT3_ZERO, implementer->sourceRadius, ScenePrimitiveFlags::None);
        lightSphereView->transformMesh = entityDb->Query<ECS::EntityViewTransform>(meshEgid)->transform;
        lightSphereView->transformLight = implementer;
        return egid;
    }

    EGID FlyCameraEntity(EntityDatabase* entityDb,
        RenderViewType type,
        const uint4& desiredRect,
        bool isWindowTarget,
        const float3& position,
        const float3& rotation,
        float moveSpeed,
        float fieldOfView,
        float zNear,
        float zFar,
        float moveSmoothing,
        float rotationSmoothing,
        float sensitivity)
    {
        auto egid = entityDb->ReserveEntityId((uint)ENTITY_GROUPS::ACTIVE);
        auto implementer = entityDb->ReserveImplementer<ImplementerFlyCamera>();
        EntityViewTransform(entityDb, implementer, egid, position, rotation, PK_FLOAT3_ONE, {});
        EntityViewRenderView(entityDb, implementer, egid, type, desiredRect, isWindowTarget);
        EntityViewFlyCamera(entityDb, implementer, egid, moveSpeed, fieldOfView, zNear, zFar, moveSmoothing, rotationSmoothing, sensitivity);
        return egid;
    }
}