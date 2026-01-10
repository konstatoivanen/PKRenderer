#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Math/FunctionsIntersect.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/MeshStaticAsset.h"
#include "App/ECS/ImplementerMeshStatic.h"
#include "App/ECS/ImplementerFlyCamera.h"
#include "App/ECS/ImplementerLight.h"
#include "App/ECS/EntityViewLightSphereTransforms.h"
#include "App/ECS/EntityViewRenderView.h"
#include "App/ECS/EntityViewFlyCamera.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "App/ECS/EntityViewMeshStatic.h"
#include "App/ECS/EntityViewLight.h"
#include "App/ECS/EntityViewFlyCamera.h"
#include "App/Renderer/HashCache.h"
#include "Builders.h"

namespace PK::App::EntityBuilders
{
    static BoundingBox GetSubmeshRangeBounds(MeshStatic* mesh, const std::initializer_list<MaterialTarget>& materials)
    {
        auto bounds = BoundingBox::GetMinBounds();

        for (auto& target : materials)
        {
            auto sm = mesh->GetSubmesh(target.submesh);
            Math::BoundsEncapsulate(&bounds, sm->bounds);
        }

        return bounds;
    }

    template<typename T>
    static void CreateEntityViewTransform(EntityDatabase* entityDb,
        T* implementer,
        const EGID& egid,
        const float3& position,
        const float3& rotation,
        const float3& scale,
        const BoundingBox& localBounds)
    {
        entityDb->ReserveView(implementer, egid, &EntityViewTransform::bounds, &EntityViewTransform::transform);
        implementer->localAABB = localBounds;
        implementer->position = position;
        implementer->rotation = glm::quat(rotation);
        implementer->scale = scale;
    }

    template<typename T>
    static void CreateEntityViewScenePrimitive(EntityDatabase* entityDb,
        T* implementer,
        const EGID& egid,
        ScenePrimitiveFlags flags)
    {
        entityDb->ReserveView(implementer, egid, &EntityViewScenePrimitive::primitive, &EntityViewScenePrimitive::bounds);
        implementer->flags = flags;
    }

    template<typename T>
    static void CreateEntityViewMeshStatic(EntityDatabase* entityDb,
        T* implementer,
        const EGID& egid,
        MeshStatic* staticMesh,
        const std::initializer_list<MaterialTarget>& materials)
    {
        entityDb->ReserveView(implementer, egid, &EntityViewMeshStatic::primitive, &EntityViewMeshStatic::materials, &EntityViewMeshStatic::staticMesh, &EntityViewMeshStatic::transform);
        implementer->materials.CopyFrom(materials);
        implementer->sharedMesh = staticMesh;
    }

    template<typename T>
    static void CreateEntityViewLight(EntityDatabase* entityDb,
        T* implementer,
        const EGID& egid,
        LightType type,
        LightCookie cookie,
        const color& color,
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
    static void CreateEntityViewLightPrimitive(EntityDatabase* entityDb,
        T* implementer,
        const EGID& egid,
        LightType type,
        LightCookie cookie,
        const color& color,
        float angle,
        float radius,
        bool castShadows)
    {
        // Light radius based on phyiscal attenuation at minAtten cutoff.
        const auto minAtten = 0.2f;
        auto intensity = glm::compMax(color);
        radius = radius < 0.0f ? (intensity * intensity) / (minAtten * minAtten) : radius;

        auto flags = ScenePrimitiveFlags::Light;

        if (castShadows)
        {
            flags = flags | ScenePrimitiveFlags::CastShadows;
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

        CreateEntityViewScenePrimitive(entityDb, implementer, egid, flags);
        CreateEntityViewLight(entityDb, implementer, egid, type, cookie, color, radius, angle);
    }

    template<typename T>
    static void CreateEntityViewRenderView(EntityDatabase* entityDb, T* implementer, const EGID& egid, const FixedString16& name, const uint4& desiredRect, bool isWindowTarget)
    {
        entityDb->ReserveView(implementer, egid, 
            &EntityViewRenderView::transform, 
            &EntityViewRenderView::projection, 
            &EntityViewRenderView::renderView, 
            &EntityViewRenderView::input,
            &EntityViewRenderView::time);
        implementer->name = name;
        implementer->desiredRect = desiredRect;
        implementer->isWindowTarget = isWindowTarget;
    }

    template<typename T>
    static void CreateEntityViewFlyCamera(EntityDatabase* entityDb,
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
        entityDb->ReserveView(implementer, egid, 
            &EntityViewFlyCamera::transform, 
            &EntityViewFlyCamera::projection, 
            &EntityViewFlyCamera::input, 
            &EntityViewFlyCamera::time, 
            &EntityViewFlyCamera::flyCamera);
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

    EGID CreateEntityMeshStatic(EntityDatabase* entityDb,
        MeshStatic* mesh,
        const std::initializer_list<MaterialTarget>& materials,
        const float3& position,
        const float3& rotation,
        float size,
        ScenePrimitiveFlags flags)
    {
        auto egid = entityDb->ReserveEntityId((uint)ENTITY_GROUPS::ACTIVE);
        auto implementer = entityDb->ReserveImplementer<ImplementerMeshStatic>();
        CreateEntityViewTransform(entityDb, implementer, egid, position, rotation, PK_FLOAT3_ONE * size, GetSubmeshRangeBounds(mesh, materials));
        CreateEntityViewScenePrimitive(entityDb, implementer, egid, flags | ScenePrimitiveFlags::Mesh);
        CreateEntityViewMeshStatic(entityDb, implementer, egid, mesh, materials);
        return egid;
    }

    EGID CreateEntityLight(EntityDatabase* entityDb,
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
        CreateEntityViewTransform(entityDb, implementer, egid, position, rotation, PK_FLOAT3_ONE, {});
        CreateEntityViewLightPrimitive(entityDb, implementer, egid, type, cookie, color, angle, radius, castShadows);
        return egid;
    }

    EGID CreateEntityLightSphere(EntityDatabase* entityDb,
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
        CreateEntityViewTransform(entityDb, implementer, egid, position, PK_FLOAT3_ZERO, PK_FLOAT3_ONE, {});
        CreateEntityViewLightPrimitive(entityDb, implementer, egid, type, cookie, color, 90.0f, 20.0f, castShadows);
        auto lightSphereView = entityDb->ReserveView<EntityViewLightSphereTransforms>(egid);

        implementer->sourceRadius = kLightSourceRadius;

        auto mesh = assetDatabase->Find<MeshStaticAsset>("Primitive_Sphere")->GetMeshStatic();
        auto shader = assetDatabase->Find<ShaderAsset>("MS_Mat_Unlit_Color");
        auto material = assetDatabase->CreateVirtual<Material>(("M_Point_Light_" + std::to_string(egid.entityID())).c_str(), shader, nullptr);
        material->Set<float4>(HashCache::Get()->_Color, color);
        material->Set<float4>(HashCache::Get()->_ColorVoxelize, PK_COLOR_BLACK);

        auto meshEgid = EntityBuilders::CreateEntityMeshStatic(entityDb, mesh, { { material, 0 } }, position, PK_FLOAT3_ZERO, implementer->sourceRadius, ScenePrimitiveFlags::None);
        lightSphereView->transformMesh = entityDb->Query<EntityViewTransform>(meshEgid)->transform;
        lightSphereView->transformLight = implementer;
        return egid;
    }

    EGID CreateEntityFlyCamera(EntityDatabase* entityDb,
        const FixedString16& name,
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
        CreateEntityViewTransform(entityDb, implementer, egid, position, rotation, PK_FLOAT3_ONE, {});
        CreateEntityViewRenderView(entityDb, implementer, egid, name, desiredRect, isWindowTarget);
        CreateEntityViewFlyCamera(entityDb, implementer, egid, moveSpeed, fieldOfView, zNear, zFar, moveSmoothing, rotationSmoothing, sensitivity);
        return egid;
    }
}
