#include "PrecompiledHeader.h"
#include "EngineDebug.h"
#include "ECS/Builders/Builders.h"
#include "Rendering/Objects/VirtualMesh.h"
#include "Rendering/MeshUtility.h"
#include "Rendering/HashCache.h"
#include "Math/FunctionsMisc.h"
#include "Math/FunctionsColor.h"
#include "Rendering/HashCache.h"

namespace PK::ECS::Engines
{
    using namespace PK::Utilities;
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::Services;
    using namespace PK::Rendering;
    using namespace PK::Rendering::Structs;
    using namespace PK::Rendering::Objects;

    EngineDebug::EngineDebug(AssetDatabase* assetDatabase, EntityDatabase* entityDb, const ApplicationConfig* config)
    {
        m_entityDb = entityDb;
        m_assetDatabase = assetDatabase;

        BufferLayout defaultLayout =
        {
            { ElementType::Float3, PK_VS_NORMAL },
            { ElementType::Float4, PK_VS_TANGENT },
            { ElementType::Float2, PK_VS_TEXCOORD0 },
        };

        BufferLayout positionLayout = { { ElementType::Float3, PK_VS_POSITION } };

        auto virtualVBuffer0 = Buffer::Create(defaultLayout, 2000000, BufferUsage::SparseVertex, "VirtualMesh.VertexBuffer0");
        auto virtualVBuffer1 = Buffer::Create(positionLayout, 2000000, BufferUsage::SparseVertex | BufferUsage::Storage, "VirtualMesh.VertexBuffer1");
        auto virtualIBuffer = Buffer::Create(ElementType::Uint, 2000000, BufferUsage::SparseIndex | BufferUsage::Storage, "VirtualMesh.IndexBuffer");
        m_virtualBaseMesh = CreateRef<Mesh>(virtualVBuffer0, virtualIBuffer);
        m_virtualBaseMesh->AddVertexBuffer(virtualVBuffer1);

        // @TODO replace this crap with this extention when it comes out of beta.
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_ray_tracing_position_fetch.html
        auto hash = HashCache::Get();
        GraphicsAPI::SetBuffer(hash->pk_RT_Vertices, virtualVBuffer1.get());
        GraphicsAPI::SetBuffer(hash->pk_RT_Indices, virtualIBuffer.get());

        auto columnMesh = assetDatabase->Load<VirtualMesh>("res/models/MDL_Columns.pkmesh", &m_virtualBaseMesh);
        auto rocksMesh = assetDatabase->Load<VirtualMesh>("res/models/MDL_Rocks.pkmesh", &m_virtualBaseMesh);
        auto sphereMesh = assetDatabase->RegisterProcedural<VirtualMesh>("Primitive_Sphere", MeshUtility::GetSphere(m_virtualBaseMesh, PK_FLOAT3_ZERO, 1.0f));
        auto planeMesh = assetDatabase->RegisterProcedural<VirtualMesh>("Primitive_Plane16x16", MeshUtility::GetPlane(m_virtualBaseMesh, PK_FLOAT2_ZERO, PK_FLOAT2_ONE, { 16, 16 }));

        auto materialSand = assetDatabase->Load<Material>("res/materials/M_Sand.material");
        auto materialAsphalt = assetDatabase->Load<Material>("res/materials/M_Asphalt.material");
        auto materialMarble = assetDatabase->Load<Material>("res/materials/M_Marble.material");
        auto materialPlaster = assetDatabase->Load<Material>("res/materials/M_Plaster.material");

        auto minpos = float3(-70, -6, -70);
        auto maxpos = float3(70, -4, 70);

        srand(config->RandomSeed);

        Builders::BuildMeshRenderableEntity(m_entityDb, planeMesh, { {materialSand,0} }, { 0, -5, 0 }, { 90, 0, 0 }, 80.0f);
        Builders::BuildMeshRenderableEntity(m_entityDb, columnMesh, { {materialAsphalt,0} }, { -20, 5, -20 }, PK_FLOAT3_ZERO, 3.0f);

        auto submeshCount = rocksMesh->GetSubmeshCount();

        for (auto i = 0; i < 128; ++i)
        {
            auto submesh = Functions::RandomRangeUint(0, submeshCount);
            auto pos = Functions::RandomRangeFloat3(minpos, maxpos);
            auto rot = Functions::RandomEuler();
            auto size = Functions::RandomRangeFloat(1.0f, 3.0f);
            Builders::BuildMeshRenderableEntity(m_entityDb, rocksMesh, { {materialMarble,submesh} }, pos, rot, size);
        }

        for (auto i = 0; i < 128; ++i)
        {
            auto submesh = Functions::RandomRangeUint(0, submeshCount);
            auto pos = Functions::RandomRangeFloat3(minpos, maxpos);
            auto rot = Functions::RandomEuler();
            auto size = Functions::RandomRangeFloat(1.0f, 3.0f);
            Builders::BuildMeshRenderableEntity(m_entityDb, rocksMesh, { {materialPlaster,submesh} }, pos, rot, size);
        }

        for (uint32_t i = 0u; i < config->LightCount; ++i)
        {
            Builders::BuildLightSphereRenderableEntity(
                m_entityDb,
                m_assetDatabase,
                Functions::RandomRangeFloat3(minpos, maxpos) + PK_FLOAT3_UP,
                i % 2 == 0 ? LightType::Spot : LightType::Point,
                Cookie::Circle0,
                Functions::HueToRGB(Functions::RandomRangeFloat(0.0f, 1.0f)) * Functions::RandomRangeFloat(2.0f, 7.0f),
                true);
        }

        auto color = glm::log(Functions::HexToRGB(0x6D563DFF) * 8.0f); // 0x6D563DFF //0x66D1FFFF //0xF78B3DFF //0xFFA575FF
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
    }
}