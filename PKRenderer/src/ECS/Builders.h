#pragma once
#include "Utilities/ForwardDeclareUtility.h"
#include "Math/Types.h"
#include "Rendering/Objects/Material.h"
#include "Rendering/EntityEnums.h"
#include "ECS/EGID.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, struct StaticMesh)

namespace PK::ECS::Build
{
    EGID StaticMeshEntity(EntityDatabase* entityDb,
        Rendering::Objects::StaticMesh* mesh,
        const std::initializer_list<Rendering::Objects::MaterialTarget>& materials,
        const Math::float3& position,
        const Math::float3& rotation,
        float size = 1.0f,
        Rendering::ScenePrimitiveFlags flags = Rendering::ScenePrimitiveFlags::DefaultMesh);

    EGID LightEntity(EntityDatabase* entityDb,
        Core::Assets::AssetDatabase* assetDatabase,
        const Math::float3& position,
        const Math::float3& rotation,
        Rendering::LightType type,
        Rendering::LightCookie cookie,
        const Math::color& color,
        float angle,
        float radius,
        bool castShadows);

    EGID LightSphereEntity(EntityDatabase* entityDb,
        Core::Assets::AssetDatabase* assetDatabase,
        const Math::float3& position,
        Rendering::LightType type,
        Rendering::LightCookie cookie,
        const Math::color& color,
        bool castShadows);

    EGID FlyCameraEntity(EntityDatabase* entityDb,
        Rendering::RenderViewType type,
        const Math::uint4& desiredRect,
        bool isWindowTarget,
        const Math::float3& position,
        const Math::float3& rotation,
        float moveSpeed,
        float fieldOfView,
        float zNear,
        float zFar,
        float moveSmoothing,
        float rotationSmoothing,
        float sensitivity);
}