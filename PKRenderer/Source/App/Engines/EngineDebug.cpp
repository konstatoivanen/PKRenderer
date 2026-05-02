#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Math/Random.h"
#include "Core/Math/Extended.h"
#include "Core/Math/Projection.h"
#include "Core/Math/Color.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/Mesh.h"
#include "Core/Rendering/MeshUtilities.h"
#include "Core/Rendering/Window.h"
#include "Core/IApplication.h"
#include "App/Engines/EngineTime.h"
#include "App/ECS/Builders.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "App/ECS/EntityViewFlyCamera.h"
#include "App/ECS/EntityViewRenderView.h"
#include "App/Renderer/IGUIRenderer.h"
#include "App/Renderer/HashCache.h"
#include "EngineDebug.h"

namespace PK::App
{
    EngineDebug::EngineDebug(AssetDatabase* assetDatabase, EntityDatabase* entityDb, MeshStaticAllocator* meshAllocator)
    {
        m_entityDb = entityDb;
        m_assetDatabase = assetDatabase;
        auto config = assetDatabase->Load<EngineDebugConfig>("Content/Configs/DebugEngine.cfg");

        auto columnMesh = assetDatabase->Load<MeshStatic>("Content/Models/MDL_Columns.pkmesh", CacheMode::Shared);
        auto rocksMesh = assetDatabase->Load<MeshStatic>("Content/Models/MDL_Rocks.pkmesh", CacheMode::Shared);
        // @TODO move these to some global resources initializer.
        auto sphereMesh = assetDatabase->CreateVirtual<MeshStatic>("Primitive_Sphere", MeshUtilities::CreateSphereMeshStatic(meshAllocator, PK_FLOAT3_ZERO, 1.0f));
        auto planeMesh = assetDatabase->CreateVirtual<MeshStatic>("Primitive_Plane", MeshUtilities::CreatePlaneMeshStatic(meshAllocator, PK_FLOAT2_ZERO, PK_FLOAT2_ONE, { 16, 16 }));

        auto materialSand = assetDatabase->Load<Material>("Content/Materials/M_Sand.material", CacheMode::Shared);
        auto materialAsphalt = assetDatabase->Load<Material>("Content/Materials/M_Asphalt.material", CacheMode::Shared);
        auto materialMarble = assetDatabase->Load<Material>("Content/Materials/M_Marble.material", CacheMode::Shared);
        auto materialPlaster = assetDatabase->Load<Material>("Content/Materials/M_Plaster.material", CacheMode::Shared);

        auto minpos = float3(-70, -6, -70);
        auto maxpos = float3(+70, -4, +70);

        EntityBuilders::CreateEntityMeshStatic(m_entityDb, planeMesh, { {materialSand,0} }, { 0, -5, 0 }, float3(90, 0, 0) * PK_FLOAT_DEG2RAD, 80.0f);
        EntityBuilders::CreateEntityMeshStatic(m_entityDb, columnMesh, { {materialAsphalt,0} }, { -20, 5, -20 }, PK_FLOAT3_ZERO, 3.0f);

        auto submeshCount = rocksMesh->GetSubmeshCount();

        for (auto i = 0; i < 128; ++i)
        {
            auto submesh = math::randomRange(0u, submeshCount);
            auto pos = math::halton(i, uint3(7,11,17)) * (maxpos - minpos) + minpos;
            auto rot = math::randomRadianFloat3();
            auto size = math::randomRange(1.0f, 3.0f);
            EntityBuilders::CreateEntityMeshStatic(m_entityDb, rocksMesh, { { materialMarble,submesh} }, pos, rot, size);
        }

        for (auto i = 0; i < 128; ++i)
        {
            auto submesh = math::randomRange(0u, submeshCount);
            auto pos = math::halton(i + 128, uint3(7,11,17)) * (maxpos - minpos) + minpos;
            auto rot = math::randomRadianFloat3();
            auto size = math::randomRange(1.0f, 3.0f);
            EntityBuilders::CreateEntityMeshStatic(m_entityDb, rocksMesh, { {materialPlaster,submesh} }, pos, rot, size);
        }

        for (uint32_t i = 0u; i < config->LightCount; ++i)
        {
            auto pos = math::randomRange(minpos, maxpos) + PK_FLOAT3_UP * 4.0f;
            auto type = i % 2 == 0 ? LightType::Spot : LightType::Point;
            auto color = math::hueToRgb(math::randomRange(0.0f, 1.0f)) * math::randomRange(8.0f, 128.0f);
            EntityBuilders::CreateEntityLightSphere(m_entityDb, m_assetDatabase, pos, type, LightCookie::Circle0, color, true);
        }

        auto color = math::hexToRgb<float>(0xFF5E19FFu) * 24.0f; // 0x6D563DFF //0x66D1FFFF //0xF78B3DFF //0xFFA575FF

        EntityBuilders::CreateEntityLight(m_entityDb, 
            PK_FLOAT3_ZERO, 
            float3(10, -35, 0) * PK_FLOAT_DEG2RAD, 
            LightType::Directional, 
            LightCookie::Circle0, 
            color, 
            90.0f, 
            1000.0f, 
            true);

        m_cameraEgid = EntityBuilders::CreateEntityFlyCamera(m_entityDb,
            "Scene",
            PK_UINT4_MAX,
            /*isWindowTarget*/ true,
            config->CameraStartPosition,
            config->CameraStartRotation,
            config->CameraSpeed,
            config->CameraFov,
            config->CameraZNear,
            config->CameraZFar,
            config->CameraMoveSmoothing,
            config->CameraLookSmoothing,
            config->CameraLookSensitivity);

        auto cameraRenderView = m_entityDb->Query<EntityViewRenderView>(m_cameraEgid);
        cameraRenderView->renderView->settingsRef = &config->ViewSettings;
    }

    void EngineDebug::OnStepFrameUpdate([[maybe_unused]] FrameContext* ctx)
    {
        /*
        auto lights = m_entityDb->Query<EntityViews::LightSphere>((int)ENTITY_GROUPS::ACTIVE);
        auto time = Application::GetService<Time>()->GetTime();

        for (auto i = 0; i < lights.count; ++i)
        {
            // auto ypos = math::sin(time * 2 + ((float)i * 4 / lights.count));
            auto rotation = quaternion(float3(0, time + float(i), 0));
            lights[i].transformLight->rotation = rotation;
            lights[i].transformMesh->rotation = rotation;
            //lights[i].transformLight->position.y = ypos;
            //lights[i].transformMesh->position.y = ypos;
        }

        return;

        auto meshes = m_entityDb->Query<EntityViews::MeshRenderable>((int)ENTITY_GROUPS::ACTIVE);

        for (auto i = 0; i < meshes.count; ++i)
        {
            meshes[i].transform->position.y = math::sin(time + (10 * (float)i / meshes.count)) * 10;
        }
        */
    }

    void EngineDebug::Step(IGizmosRenderer* gizmos)
    {
        auto cullables = m_entityDb->Query<EntityViewScenePrimitive>((uint32_t)ENTITY_GROUPS::ACTIVE);

        gizmos->GizmosSetColor(PK_COLOR_GREEN);

        for (auto i = 0u; i < cullables.count; ++i)
        {
            auto cullable = &cullables[i];
            auto flags = cullable->primitive->flags;

            if ((flags & ScenePrimitiveFlags::Mesh) == 0u)
            {
                continue;
            }

            gizmos->GizmosDrawBounds(cullables[i].bounds->worldAABB);
        }

        auto offset = float3(-100, 50, 0);
        auto time = IApplication::Get()->GetService<EngineTime>()->GetTime() * 0.1f;
        auto aspect = IApplication::Get()->GetPrimaryWindow()->GetAspectRatio();
        auto viewToClip = math::perspective(50.0f, aspect, 0.2f, 25.0f);
        auto worldToView = math::transformTRSInverse(offset, { 0, time, 0 }, PK_FLOAT3_ONE);
        auto worldToClip = viewToClip * worldToView;

        gizmos->GizmosSetColor(PK_COLOR_GREEN);
        gizmos->GizmosDrawFrustrum(worldToClip);

        float4x4 localToWorld = math::transformTRS(offset, float3(35, -35, 0) * PK_FLOAT_DEG2RAD, PK_FLOAT3_ONE);
        float4x4 worldToLocal = math::inverse(localToWorld);
        float4x4 invvp = math::inverse(worldToClip);
        float4x4 cascades[4];
        float zplanes[5];

        ShadowCascadeCreateInfo cascadeInfo{};
        cascadeInfo.worldToLocal = worldToLocal;
        cascadeInfo.clipToWorld = invvp;
        cascadeInfo.splitPlanes = zplanes;
        cascadeInfo.nearPlaneOffset = 0.0f;
        cascadeInfo.resolution = 1024;
        cascadeInfo.count = 4;
        math::cascadeDepths<float, 5>(0.2f, 25.0f, 0.5f, zplanes);
        math::composeShadowCascadeMatrices(cascadeInfo, cascades);

        for (auto i = 0; i < 4; ++i)
        {
            gizmos->GizmosSetColor(PK_COLOR_GREEN);
            gizmos->GizmosDrawFrustrum(cascades[i]);
        }
    }

    void EngineDebug::Step(AssetImportEvent<EngineDebugConfig>* token)
    {
        auto entity = m_entityDb->Query<EntityViewFlyCamera>(m_cameraEgid);
        auto config = token->asset;
        entity->projection->fieldOfView = config->CameraFov;
        entity->projection->zNear = config->CameraZNear;
        entity->projection->zFar = config->CameraZFar;
        entity->flyCamera->moveSpeed = config->CameraSpeed;
        entity->flyCamera->moveSmoothing = config->CameraMoveSmoothing;
        entity->flyCamera->rotationSmoothing = config->CameraLookSmoothing;
        entity->flyCamera->sensitivity = config->CameraLookSensitivity;
        entity->flyCamera->snapshotPosition = config->CameraStartPosition;
        entity->flyCamera->snapshotRotation = config->CameraStartRotation;
    }
}
