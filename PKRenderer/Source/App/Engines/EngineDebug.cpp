#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Math/FunctionsColor.h"
#include "Core/Math/FunctionsMatrix.h"
#include "Core/Math/FunctionsIntersect.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/MeshStaticAsset.h"
#include "Core/Rendering/MeshStaticCollection.h"
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
    EngineDebug::EngineDebug(AssetDatabase* assetDatabase, EntityDatabase* entityDb, MeshStaticCollection* baseMesh)
    {
        m_entityDb = entityDb;
        m_assetDatabase = assetDatabase;
        auto config = assetDatabase->Load<EngineDebugConfig>("Content/Configs/DebugEngine.cfg");

        auto columnMesh = assetDatabase->Load<MeshStaticAsset>("Content/Models/MDL_Columns.pkmesh", baseMesh);
        auto rocksMesh = assetDatabase->Load<MeshStaticAsset>("Content/Models/MDL_Rocks.pkmesh", baseMesh);
        [[maybe_unused]] auto sphereMesh = assetDatabase->Register<MeshStaticAsset>("Primitive_Sphere", MeshUtilities::CreateSphereMeshStaticAsset(baseMesh, PK_FLOAT3_ZERO, 1.0f));
        auto planeMesh = assetDatabase->Register<MeshStaticAsset>("Primitive_Plane16x16", MeshUtilities::CreatePlaneMeshStaticAsset(baseMesh, PK_FLOAT2_ZERO, PK_FLOAT2_ONE, { 16, 16 }));

        auto materialSand = assetDatabase->Load<Material>("Content/Materials/M_Sand.material");
        auto materialAsphalt = assetDatabase->Load<Material>("Content/Materials/M_Asphalt.material");
        auto materialMarble = assetDatabase->Load<Material>("Content/Materials/M_Marble.material");
        auto materialPlaster = assetDatabase->Load<Material>("Content/Materials/M_Plaster.material");

        auto minpos = float3(-70, -6, -70);
        auto maxpos = float3(70, -4, 70);

        EntityBuilders::CreateEntityMeshStatic(m_entityDb, planeMesh->GetMeshStatic(), { {materialSand,0} }, { 0, -5, 0 }, float3(90, 0, 0) * PK_FLOAT_DEG2RAD, 80.0f);
        EntityBuilders::CreateEntityMeshStatic(m_entityDb, columnMesh->GetMeshStatic(), { {materialAsphalt,0} }, { -20, 5, -20 }, PK_FLOAT3_ZERO, 3.0f);

        auto submeshCount = rocksMesh->GetSubmeshCount();

        for (auto i = 0; i < 128; ++i)
        {
            auto submesh = Math::RandomRangeUint(0, submeshCount);
            auto pos = Math::RandomRangeFloat3(minpos, maxpos);
            auto rot = Math::RandomEuler() * PK_FLOAT_DEG2RAD;
            auto size = Math::RandomRangeFloat(1.0f, 3.0f);
            EntityBuilders::CreateEntityMeshStatic(m_entityDb, rocksMesh->GetMeshStatic(), { {materialMarble,submesh} }, pos, rot, size);
        }

        for (auto i = 0; i < 128; ++i)
        {
            auto submesh = Math::RandomRangeUint(0, submeshCount);
            auto pos = Math::RandomRangeFloat3(minpos, maxpos);
            auto rot = Math::RandomEuler() * PK_FLOAT_DEG2RAD;
            auto size = Math::RandomRangeFloat(1.0f, 3.0f);
            EntityBuilders::CreateEntityMeshStatic(m_entityDb, rocksMesh->GetMeshStatic(), { {materialPlaster,submesh} }, pos, rot, size);
        }

        for (uint32_t i = 0u; i < config->LightCount; ++i)
        {
            auto pos = Math::RandomRangeFloat3(minpos, maxpos) + PK_FLOAT3_UP * 4.0f;
            auto type = i % 2 == 0 ? LightType::Spot : LightType::Point;
            auto color = Math::HueToRGB(Math::RandomRangeFloat(0.0f, 1.0f)) * Math::RandomRangeFloat(8.0f, 128.0f);
            EntityBuilders::CreateEntityLightSphere(m_entityDb, m_assetDatabase, pos, type, LightCookie::Circle0, color, true);
        }

        auto color = Math::HexToRGB(0x6D563DFF) * 32.0f; // 0x6D563DFF //0x66D1FFFF //0xF78B3DFF //0xFFA575FF

        EntityBuilders::CreateEntityLight(m_entityDb, 
            PK_FLOAT3_ZERO, 
            float3(25, -35, 0) * PK_FLOAT_DEG2RAD, 
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

    void EngineDebug::OnApplicationUpdateEngines()
    {
        /*
        auto lights = m_entityDb->Query<EntityViews::LightSphere>((int)ENTITY_GROUPS::ACTIVE);
        auto time = Application::GetService<Time>()->GetTime();

        for (auto i = 0; i < lights.count; ++i)
        {
            // auto ypos = sin(time * 2 + ((float)i * 4 / lights.count));
            auto rotation = glm::quat(float3(0, time + float(i), 0));
            lights[i].transformLight->rotation = rotation;
            lights[i].transformMesh->rotation = rotation;
            //lights[i].transformLight->position.y = ypos;
            //lights[i].transformMesh->position.y = ypos;
        }

        return;

        auto meshes = m_entityDb->Query<EntityViews::MeshRenderable>((int)ENTITY_GROUPS::ACTIVE);

        for (auto i = 0; i < meshes.count; ++i)
        {
            meshes[i].transform->position.y = sin(time + (10 * (float)i / meshes.count)) * 10;
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
        auto viewToClip = Math::GetPerspective(50.0f, aspect, 0.2f, 25.0f);
        auto worldToView = Math::GetMatrixInvTRS(offset, { 0, time, 0 }, PK_FLOAT3_ONE);
        auto worldToClip = viewToClip * worldToView;

        gizmos->GizmosSetColor(PK_COLOR_GREEN);
        gizmos->GizmosDrawFrustrum(worldToClip);

        float4x4 localToWorld = Math::GetMatrixTRS(offset, float3(35, -35, 0) * PK_FLOAT_DEG2RAD, PK_FLOAT3_ONE);
        float4x4 worldToLocal = glm::inverse(localToWorld);
        float4x4 invvp = glm::inverse(worldToClip);
        float4x4 cascades[4];
        float zplanes[5];

        ShadowCascadeCreateInfo cascadeInfo{};
        cascadeInfo.worldToLocal = worldToLocal;
        cascadeInfo.clipToWorld = invvp;
        cascadeInfo.splitPlanes = zplanes;
        cascadeInfo.nearPlaneOffset = 0.0f;
        cascadeInfo.resolution = 1024;
        cascadeInfo.count = 4;
        Math::GetCascadeDepths(0.2f, 25.0f, 0.5f, zplanes, 5);
        Math::GetShadowCascadeMatrices(cascadeInfo, cascades);

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
        entity->flyCamera->snashotPosition = config->CameraStartPosition;
        entity->flyCamera->snashotRotation = config->CameraStartRotation;
    }
}