#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Math/Random.h"
#include "Core/Math/Projection.h"
#include "Core/Math/Color.h"
#include "Core/Math/Extended.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/Mesh.h"
#include "Core/Rendering/MeshUtilities.h"
#include "Core/Rendering/TextureAsset.h"
#include "Core/Rendering/Window.h"
#include "Core/IApplication.h"
#include "App/Engines/EngineTime.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "App/ECS/EntityViewFlyCamera.h"
#include "App/ECS/EntityViewRenderView.h"
#include "App/Renderer/IGUIRenderer.h"
#include "App/Renderer/Passes/PassLights.h"
#include "App/Renderer/HashCache.h"
#include "App/ECS/EntityLight.h"
#include "App/ECS/EntityMeshStatic.h"
#include "App/ECS/EntityLightSphere.h"
#include "App/ECS/EntityFlyCamera.h"
#include "EngineDebug.h"

namespace PK::App
{
    EngineDebug::EngineDebug(AssetDatabase* assetDatabase, EntityDatabase* entityDb, MeshStaticAllocator* meshAllocator)
    {
        m_entityDb = entityDb;
        m_assetDatabase = assetDatabase;
        auto config = assetDatabase->Load<Config<EngineDebugConfig>>("Content/Configs/DebugEngine.cfg");

        auto columnMesh = assetDatabase->Load<MeshStatic>("Content/Models/MDL_Columns.pkmesh", CacheMode::Shared);
        auto rocksMesh = assetDatabase->Load<MeshStatic>("Content/Models/MDL_Rocks.pkmesh", CacheMode::Shared);
        // @TODO move these to some global resources initializer.
        auto sphereMesh = assetDatabase->CreateVirtual<MeshStatic>("Primitive_Sphere", MeshUtilities::CreateSphereMeshStatic(meshAllocator, PK_FLOAT3_ZERO, 1.0f));
        auto planeMesh = assetDatabase->CreateVirtual<MeshStatic>("Primitive_Plane", MeshUtilities::CreatePlaneMeshStatic(meshAllocator, PK_FLOAT2_ZERO, PK_FLOAT2_ONE, { 16, 16 }));

        auto materialSand = assetDatabase->Load<Material>("Content/Materials/M_Sand.material", CacheMode::Shared);
        auto materialAsphalt = assetDatabase->Load<Material>("Content/Materials/M_Asphalt.material", CacheMode::Shared);
        auto materialMarble = assetDatabase->Load<Material>("Content/Materials/M_Marble.material", CacheMode::Shared);
        auto materialPlaster = assetDatabase->Load<Material>("Content/Materials/M_Plaster.material", CacheMode::Shared);

        auto profile = assetDatabase->Load<IESProfile>("Content/IESProfiles/IES_150W_45D.pkiesprofile", CacheMode::Shared);

        auto minpos = float3(-70, -6, -70);
        auto maxpos = float3(+70, -4, +70);

        entityDb->Reserve<EntityMeshStatic>(300u);
        entityDb->Reserve<EntityLightSphere>(32u);
        entityDb->Reserve<EntityLight>(32u);

        // Floor mesh
        {
            MaterialTarget material { materialSand, 0u };
            EntityMeshStatic::Descriptor desc;
            desc.entityName = "Floor";
            desc.entitySerialize = true;
            desc.flags = ScenePrimitiveFlags::DefaultMesh;
            desc.mesh = planeMesh;
            desc.materials = { &material, 1u };
            desc.position = { 0.0f, -5.0f, 0.0f };
            desc.rotation = { 90.0f * PK_FLOAT_DEG2RAD, 0.0f, 0.0f };
            desc.scale = 80.0f * PK_FLOAT3_ONE;
            EntityFactory<EntityMeshStatic>::Create(m_entityDb, desc);
        }

        // Columns mesh
        {
            MaterialTarget material = { materialAsphalt, 0u };
            EntityMeshStatic::Descriptor desc;
            desc.entityName = "Columns";
            desc.entitySerialize = true;
            desc.flags = ScenePrimitiveFlags::DefaultMesh;
            desc.mesh = columnMesh;
            desc.materials = { &material, 1u };
            desc.position = { -20.0f, 5.0f, -20.0f };
            desc.rotation = PK_FLOAT3_ZERO;
            desc.scale = 3.0f * PK_FLOAT3_ONE;
            EntityFactory<EntityMeshStatic>::Create(m_entityDb, desc);
        }

        // Rock meshes
        {
            auto maxsubmesh = rocksMesh->GetSubmeshCount() - 1u;

            for (auto i = 0u; i < 256u; ++i)
            {
                MaterialTarget material { i < 128u ? materialMarble : materialPlaster, math::randomRange(0u, maxsubmesh) };
                EntityMeshStatic::Descriptor desc;
                desc.entityName = FixedString32("Rock_%u", i);
                desc.entitySerialize = true;
                desc.flags = ScenePrimitiveFlags::DefaultMesh;
                desc.mesh = rocksMesh;
                desc.materials = { &material, 1u };
                desc.position = math::halton(i, uint3(7, 11, 17)) * (maxpos - minpos) + minpos;
                desc.rotation = math::randomRadianFloat3();
                desc.scale = math::randomRange(1.0f, 3.0f) * PK_FLOAT3_ONE;
                EntityFactory<EntityMeshStatic>::Create(m_entityDb, desc);
            }
        }
        
        // Local lights
        for (auto i = 0u; i < config->LightCount; ++i)
        {
            EntityLightSphere::Descriptor desc;
            desc.assetDatabase = m_assetDatabase;
            desc.type = i % 2 == 0 ? LightType::Spot : LightType::Point;
            desc.IESProfile = i % 2 == 0 ? profile : nullptr;
            desc.position = math::randomRange(minpos, maxpos) + PK_FLOAT3_UP * 4.0f;
            desc.rotation = PK_FLOAT3_ZERO;// math::randomRange(float3(0.0f, 0.0f, 0.0f), float3(0.0f, PK_FLOAT_PI * 2.0f, 0.0f));
            desc.color = math::hueToRgb(math::randomRange(0.0f, 1.0f)) * math::randomRange(8.0f, 128.0f);
            desc.angle = 90.0f;
            desc.radius = 20.0f;
            desc.sourceRadius = 0.2f;
            desc.castShadow = true;
            desc.useIESCandelas = true;
            EntityFactory<EntityLightSphere>::Create(m_entityDb, desc);
        }

        // Directional light
        {
            EntityLight::Descriptor desc;
            desc.type = LightType::Directional;
            desc.IESProfile = nullptr;
            desc.position = PK_FLOAT3_ZERO;
            desc.rotation = float3(10, -35, 0) * PK_FLOAT_DEG2RAD;
            desc.color = math::hexToRgb<float>(0xFF5E19FFu) * 24.0f; // 0x6D563DFF //0x66D1FFFF //0xF78B3DFF //0xFFA575FF
            desc.angle = 90.0f;
            desc.radius = 1000.0f;
            desc.sourceRadius = 0.1f;
            desc.castShadow = true;
            desc.useIESCandelas = false;
            EntityFactory<EntityLight>::Create(m_entityDb, desc);
        }

        // Fly camera
        {
            EntityFlyCamera::Descriptor desc;
            desc.name = "Scene";
            desc.desiredRect = PK_UINT4_MAX;
            desc.isWindowTarget = true;
            desc.position = config->CameraStartPosition;
            desc.rotation = config->CameraStartRotation;
            desc.moveSpeed = config->CameraSpeed;
            desc.fieldOfView = config->CameraFov;
            desc.zNear = config->CameraZNear;
            desc.zFar = config->CameraZFar;
            desc.moveSmoothing = config->CameraMoveSmoothing;
            desc.rotationSmoothing = config->CameraLookSmoothing;
            desc.sensitivity = config->CameraLookSensitivity;
            desc.settings = &config->ViewSettings;
            m_cameraEnityId = *EntityFactory<EntityFlyCamera>::Create(m_entityDb, desc).entityId;
        }
    }

    void EngineDebug::OnStepFrameUpdate([[maybe_unused]] FrameContext* ctx)
    {
        /*
        auto lights = m_entityDb->Query<EntityViewLightSphere>();
        auto time = Application::GetService<Time>()->GetTime();
        auto index = 0u;

        for (auto& view : lights)
        {
            // auto ypos = math::sin(time * 2 + ((float)index * 4 / lights.count));
            auto rotation = quaternion(float3(0, time + float(index), 0));
            view.transform->rotation = rotation;
            view.transform->rotation = rotation;
            //view.transform->position.y = ypos;
            //view.transform->position.y = ypos;
        }
        */
    }

    void EngineDebug::Step(IGizmosRenderer* gizmos)
    {
        auto cullables = m_entityDb->Query<EntityViewScenePrimitive>();

        gizmos->GizmosSetColor(PK_COLOR_GREEN);

        for (auto& view : cullables)
        {
            auto flags = view.primitive->flags;

            if ((flags & ScenePrimitiveFlags::Mesh) != 0u)
            {
                gizmos->GizmosDrawBounds(view.bounds->worldAABB);
            }
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
        PassLights::BuildShadowCascadeMatrices(cascadeInfo, cascades);

        for (auto i = 0; i < 4; ++i)
        {
            gizmos->GizmosSetColor(PK_COLOR_GREEN);
            gizmos->GizmosDrawFrustrum(cascades[i]);
        }
    }

    void EngineDebug::Step(AssetImportEvent<Config<EngineDebugConfig>>* token)
    {
        auto entity = m_entityDb->Query<EntityViewFlyCamera>(m_cameraEnityId);
        auto config = token->asset;
        entity.projection->fieldOfView = config->CameraFov;
        entity.projection->zNear = config->CameraZNear;
        entity.projection->zFar = config->CameraZFar;
        entity.flyCamera->moveSpeed = config->CameraSpeed;
        entity.flyCamera->moveSmoothing = config->CameraMoveSmoothing;
        entity.flyCamera->rotationSmoothing = config->CameraLookSmoothing;
        entity.flyCamera->sensitivity = config->CameraLookSensitivity;
        entity.flyCamera->snapshotPosition = config->CameraStartPosition;
        entity.flyCamera->snapshotRotation = config->CameraStartRotation;
    }
}
