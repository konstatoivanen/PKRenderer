#pragma once
#include "Core/Math/Forward.h"
#include "Core/ECS/EGID.h"
#include "Core/Rendering/Material.h"
#include "App/Renderer/EntityEnums.h"

namespace PK { struct EntityDatabase; }
namespace PK { struct MeshStatic; }

namespace PK::App::EntityBuilders
{
    EGID CreateEntityMeshStatic(EntityDatabase* entityDb,
        MeshStaticRef mesh,
        const initializer_list<MaterialTarget>& materials,
        const float3& position,
        const float3& rotation,
        float size = 1.0f,
        ScenePrimitiveFlags flags = ScenePrimitiveFlags::DefaultMesh);

    EGID CreateEntityLight(EntityDatabase* entityDb,
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
        float sensitivity);
}
