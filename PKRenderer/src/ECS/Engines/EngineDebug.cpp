#include "PrecompiledHeader.h"
#include "Math/FunctionsMisc.h"
#include "Math/FunctionsColor.h"
#include "Math/FunctionsIntersect.h"
#include "Core/Services/Time.h"
#include "ECS/Builders/Builders.h"
#include "Rendering/MeshUtilities/PrimitiveBuilders.h"
#include "Rendering/HashCache.h"
#include "EngineDebug.h"

namespace PK::ECS::Engines
{
    using namespace PK::Utilities;
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::Services;
    using namespace PK::Rendering;
    using namespace PK::Rendering::Structs;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    EngineDebug::EngineDebug(AssetDatabase* assetDatabase, EntityDatabase* entityDb, StaticSceneMesh* baseMesh, const ApplicationConfig* config)
    {
        m_entityDb = entityDb;
        m_assetDatabase = assetDatabase;

        auto columnMesh = assetDatabase->Load<VirtualStaticMesh>("res/models/MDL_Columns.pkmesh", baseMesh);
        auto rocksMesh = assetDatabase->Load<VirtualStaticMesh>("res/models/MDL_Rocks.pkmesh", baseMesh);
        auto sphereMesh = assetDatabase->RegisterProcedural<VirtualStaticMesh>("Primitive_Sphere", MeshUtilities::CreateSphereVirtualMesh(baseMesh, PK_FLOAT3_ZERO, 1.0f));
        auto planeMesh = assetDatabase->RegisterProcedural<VirtualStaticMesh>("Primitive_Plane16x16", MeshUtilities::CreatePlaneVirtualMesh(baseMesh, PK_FLOAT2_ZERO, PK_FLOAT2_ONE, { 16, 16 }));

        auto materialSand = assetDatabase->Load<Material>("res/materials/M_Sand.material");
        auto materialAsphalt = assetDatabase->Load<Material>("res/materials/M_Asphalt.material");
        auto materialMarble = assetDatabase->Load<Material>("res/materials/M_Marble.material");
        auto materialPlaster = assetDatabase->Load<Material>("res/materials/M_Plaster.material");

        auto minpos = float3(-70, -6, -70);
        auto maxpos = float3(70, -4, 70);

        srand(config->RandomSeed);

        Builders::BuildStaticMeshRenderableEntity(m_entityDb, planeMesh, { {materialSand,0} }, { 0, -5, 0 }, { 90, 0, 0 }, 80.0f);
        Builders::BuildStaticMeshRenderableEntity(m_entityDb, columnMesh, { {materialAsphalt,0} }, { -20, 5, -20 }, PK_FLOAT3_ZERO, 3.0f);

        auto submeshCount = rocksMesh->GetSubmeshCount();

        for (auto i = 0; i < 128; ++i)
        {
            auto submesh = Functions::RandomRangeUint(0, submeshCount);
            auto pos = Functions::RandomRangeFloat3(minpos, maxpos);
            auto rot = Functions::RandomEuler();
            auto size = Functions::RandomRangeFloat(1.0f, 3.0f);
            Builders::BuildStaticMeshRenderableEntity(m_entityDb, rocksMesh, { {materialMarble,submesh} }, pos, rot, size);
        }

        for (auto i = 0; i < 128; ++i)
        {
            auto submesh = Functions::RandomRangeUint(0, submeshCount);
            auto pos = Functions::RandomRangeFloat3(minpos, maxpos);
            auto rot = Functions::RandomEuler();
            auto size = Functions::RandomRangeFloat(1.0f, 3.0f);
            Builders::BuildStaticMeshRenderableEntity(m_entityDb, rocksMesh, { {materialPlaster,submesh} }, pos, rot, size);
        }

        for (uint32_t i = 0u; i < config->LightCount; ++i)
        {
            Builders::BuildLightSphereRenderableEntity(
                m_entityDb,
                m_assetDatabase,
                Functions::RandomRangeFloat3(minpos, maxpos) + PK_FLOAT3_UP,
                i % 2 == 0 ? LightType::Spot : LightType::Point,
                Cookie::Circle0,
                Functions::HueToRGB(Functions::RandomRangeFloat(0.0f, 1.0f)) * Functions::RandomRangeFloat(8.0f, 128.0f),
                true);
        }

        auto color = Functions::HexToRGB(0x6D563DFF) * 32.0f; // 0x6D563DFF //0x66D1FFFF //0xF78B3DFF //0xFFA575FF
        Builders::BuildLightRenderableEntity(m_entityDb, m_assetDatabase, PK_FLOAT3_ZERO, { 25, -35, 0 }, LightType::Directional, Cookie::Circle0, color, 90.0f, 1000.0f, true);
    }

    void EngineDebug::Step(int condition)
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

    void EngineDebug::Step(Tokens::IGizmosRenderer* gizmos)
    {
        auto cullables = m_entityDb->Query<EntityViews::BaseRenderableView>((uint32_t)ECS::ENTITY_GROUPS::ACTIVE);

        gizmos->SetColor(PK_COLOR_GREEN);

        for (auto i = 0u; i < cullables.count; ++i)
        {
            auto cullable = &cullables[i];
            auto flags = cullable->renderable->flags;

            if ((flags & RenderableFlags::Mesh) == 0u)
            {
                continue;
            }

            gizmos->DrawBounds(cullables[i].bounds->worldAABB);
        }

        auto offset = float3(-100, 50, 0);
        auto time = Application::GetService<Time>()->GetTime() * 0.1f;
        auto aspect = Application::GetPrimaryWindow()->GetAspectRatioAligned();
        auto viewToClip = Functions::GetPerspective(50.0f, aspect, 0.2f, 25.0f);
        auto worldToView = Functions::GetMatrixInvTRS(offset, { 0, time, 0 }, PK_FLOAT3_ONE);
        auto worldToClip = viewToClip * worldToView;

        gizmos->SetColor(PK_COLOR_GREEN);
        gizmos->DrawFrustrum(worldToClip);

        /*
        float n = 0.2f;
        float f = 200.0f;
        float4 exp = { 1.0f / glm::log2(f / n), -log2(n) / log2(f / n), f / n, 1.0f / n };

        gizmos->DrawLine({ -100, 25, n }, { -100, 75, n });
        gizmos->DrawLine({ -100, 25, f }, { -100, 75, f });
        gizmos->SetColor(PK_COLOR_RED);

        for (auto i = 0; i < 128; ++i)
        {
            float z = (i + 0.5f) / 128.0f;
            z = glm::sqrt(z);
            float e = n * pow(exp.z, z);
            gizmos->DrawLine({ -100, 25, e }, { -100, 50, e });
            
            z = log2(e) * exp.x + exp.y;
            z = z * z;

            e = n * pow(exp.z, glm::sqrt(z));
            gizmos->DrawLine({ -100, 50, e }, { -100, 75, e });
        }
        */
        //log2(view_z) * pk_ClipParamsExp.x + pk_ClipParamsExp.y;

        //(m[2][2] * v[2] + m[3][2]) / v[2]
        float4x4 localToWorld = Functions::GetMatrixTRS(offset, float3(35, -35, 0) * PK_FLOAT_DEG2RAD, PK_FLOAT3_ONE);
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
        Functions::GetCascadeDepths(0.2f, 25.0f, 0.5f, zplanes, 5);
        Functions::GetShadowCascadeMatrices(cascadeInfo, cascades);

        for (auto i = 0; i < 4; ++i)
        {
            gizmos->SetColor(PK_COLOR_GREEN);
            gizmos->DrawFrustrum(cascades[i]);
        }

        /*
        gizmos->SetColor(PK_COLOR_RED);
        gizmos->DrawFrustrum(worldToClip);

        auto znear = 0.2f;
        auto zfar = 25.0f;

        for (int i = 0; i < 4; ++i)
        {
            float n = Functions::CascadeDepth(znear, zfar, 0.5f, (float)i / 4);
            float f = Functions::CascadeDepth(znear, zfar, 0.5f, (float)(i + 1) / 4);

            auto viewToClipSub = Functions::GetOffsetPerspective(-1, 1, -1, 1, 50.0f, aspect, n, f);
            auto worldToClipSub = viewToClipSub * worldToView;
            gizmos->DrawFrustrum(worldToClipSub);
        }
        */
    }
}