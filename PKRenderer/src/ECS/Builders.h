#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Math/Types.h"
#include "Graphics/Material.h"
#include "Renderer/EntityEnums.h"
#include "ECS/EGID.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Graphics, struct MeshStatic)

namespace PK::ECS::Build
{
    EGID MeshStaticEntity(EntityDatabase* entityDb,
        Graphics::MeshStatic* mesh,
        const std::initializer_list<Graphics::MaterialTarget>& materials,
        const Math::float3& position,
        const Math::float3& rotation,
        float size = 1.0f,
        Renderer::ScenePrimitiveFlags flags = Renderer::ScenePrimitiveFlags::DefaultMesh);

    EGID LightEntity(EntityDatabase* entityDb,
        Core::Assets::AssetDatabase* assetDatabase,
        const Math::float3& position,
        const Math::float3& rotation,
        Renderer::LightType type,
        Renderer::LightCookie cookie,
        const Math::color& color,
        float angle,
        float radius,
        bool castShadows);

    EGID LightSphereEntity(EntityDatabase* entityDb,
        Core::Assets::AssetDatabase* assetDatabase,
        const Math::float3& position,
        Renderer::LightType type,
        Renderer::LightCookie cookie,
        const Math::color& color,
        bool castShadows);

    EGID FlyCameraEntity(EntityDatabase* entityDb,
        Renderer::RenderViewType type,
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