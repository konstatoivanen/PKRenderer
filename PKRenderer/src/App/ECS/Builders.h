#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Math/MathFwd.h"
#include "Core/ECS/EGID.h"
#include "Core/Rendering/Material.h"
#include "App/Renderer/EntityEnums.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct MeshStatic)

namespace PK::App::EntityBuilders
{
    EGID CreateEntityMeshStatic(EntityDatabase* entityDb,
        MeshStatic* mesh,
        const std::initializer_list<MaterialTarget>& materials,
        const float3& position,
        const float3& rotation,
        float size = 1.0f,
        ScenePrimitiveFlags flags = ScenePrimitiveFlags::DefaultMesh);

    EGID CreateEntityLight(EntityDatabase* entityDb,
        AssetDatabase* assetDatabase,
        const float3& position,
        const float3& rotation,
        LightType type,
        LightCookie cookie,
        const color& color,
        float angle,
        float radius,
        bool castShadows);

    EGID CreateEntityLightSphere(EntityDatabase* entityDb,
        AssetDatabase* assetDatabase,
        const float3& position,
        LightType type,
        LightCookie cookie,
        const color& color,
        bool castShadows);

    EGID CreateEntityFlyCamera(EntityDatabase* entityDb,
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
        float sensitivity);
}