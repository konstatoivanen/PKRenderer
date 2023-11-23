#include "PrecompiledHeader.h"
#include "Math/FunctionsIntersect.h"
#include "Rendering/HashCache.h"
#include "EngineGizmos.h"

namespace PK::ECS::Engines
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::Services;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::Structs;
    using namespace PK::Rendering;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    EngineGizmos::EngineGizmos(AssetDatabase* assetDatabase, Sequencer* sequencer, ApplicationConfig* config) :
        m_sequencer(sequencer)
    {
        m_enabledCPU = config->EnableGizmosCPU;
        m_enabledGPU = config->EnableGizmosGPU;
        m_gizmosShader = assetDatabase->Find<Shader>("VS_Gizmos");
        m_vertexBuffer = Buffer::Create(BufferLayout({ { ElementType::Uint4, "in_POSITION" } }), m_maxVertices, BufferUsage::DefaultVertex | BufferUsage::PersistentStage, "Gizmos.VertexBuffer");
        m_indirectVertexBuffer = Buffer::Create(BufferLayout({ { ElementType::Uint4, "in_POSITION" } }), 16384u, BufferUsage::Vertex | BufferUsage::Storage, "Gizmos.Indirect.VertexBuffer");
        m_indirectArgsBuffer = Buffer::Create(ElementType::Uint4, 1u, BufferUsage::Storage | BufferUsage::Indirect | BufferUsage::TransferDst, "Gizmos.Indirect.Arguments");
        m_fixedFunctionAttribs = m_gizmosShader->GetFixedFunctionAttributes();
        m_fixedFunctionAttribs.rasterization.polygonMode = PolygonMode::Line;
        m_fixedFunctionAttribs.rasterization.topology = Topology::LineList;
        
        auto hash = HashCache::Get();
        GraphicsAPI::SetBuffer(hash->pk_Gizmos_IndirectVertices, m_indirectVertexBuffer.get());
        GraphicsAPI::SetBuffer(hash->pk_Gizmos_IndirectArguments, m_indirectArgsBuffer.get());
    }

    void EngineGizmos::Step(Tokens::TokenRenderEvent* token, int condition)
    {
        switch ((Tokens::RenderEvent)condition)
        {
            case Tokens::RenderEvent::CollectDraws:
            {
                if (m_enabledCPU)
                {
                    m_vertexBuffer->Validate(m_vertexCount);
                    m_color = Math::PK_COLOR_WHITE;
                    m_matrix = Math::PK_FLOAT4X4_IDENTITY;
                    m_ViewToClip = token->viewToClip;
                    m_vertexCount = 0u;
                    m_maxVertices = (uint32_t)m_vertexBuffer->GetCount();
                    m_vertexView = token->cmd->BeginBufferWrite<Vertex>(m_vertexBuffer.get());
                    m_sequencer->Next<IGizmosRenderer>(this, this, 0);
                    token->cmd->EndBufferWrite(m_vertexBuffer.get());
                }

                uint4 clearValue{ 0u, 1u, 0u, 0u };
                token->cmd->UpdateBuffer(m_indirectArgsBuffer.get(), 0u, sizeof(uint4), &clearValue);
            }
            return;
            case Tokens::RenderEvent::AfterPostEffects:
            {
                if (m_enabledGPU)
                {
                    auto rect = token->gbuffers.current.color->GetRect();
                    const Buffer* vb = m_indirectVertexBuffer.get();
                    token->cmd->SetVertexBuffers(&vb, 1u);
                    token->cmd->SetShader(m_gizmosShader);
                    token->cmd->SetRenderTarget(token->gbuffers.current.color);
                    token->cmd->SetViewPort(rect);
                    token->cmd->SetScissor(rect);
                    token->cmd->SetFixedStateAttributes(&m_fixedFunctionAttribs);
                    token->cmd->DrawIndirect(m_indirectArgsBuffer.get(), 0u, 1u, sizeof(uint4));
                }

                if (m_enabledCPU && m_vertexCount >= 2)
                {
                    auto rect = token->gbuffers.current.color->GetRect();
                    const Buffer* vb = m_vertexBuffer.get();
                    token->cmd->SetVertexBuffers(&vb, 1u);
                    token->cmd->SetShader(m_gizmosShader);
                    token->cmd->SetRenderTarget(token->gbuffers.current.color);
                    token->cmd->SetViewPort(rect);
                    token->cmd->SetScissor(rect);
                    token->cmd->SetFixedStateAttributes(&m_fixedFunctionAttribs);
                    token->cmd->Draw(glm::min(m_vertexCount, m_maxVertices), 1u, 0u, 0u);
                }
            }
            return;
        }
    }

    void EngineGizmos::Step(Core::Services::AssetImportToken<Core::ApplicationConfig>* token)
    {
        m_enabledCPU = token->asset->EnableGizmosCPU;
        m_enabledGPU = token->asset->EnableGizmosGPU;
    }

    void EngineGizmos::Step(Core::TokenConsoleCommand* token)
    {
        if (!token->isConsumed && token->argument == "toggle_gizmos_cpu")
        {
            m_enabledCPU ^= true;
            token->isConsumed = true;
            PK_LOG_INFO("Gizmos CPU %s", (m_enabledCPU ? "Enabled" : "Disabled"));
        }

        if (!token->isConsumed && token->argument == "toggle_gizmos_gpu")
        {
            m_enabledGPU ^= true;
            token->isConsumed = true;
            PK_LOG_INFO("Gizmos GPU %s", (m_enabledGPU ? "Enabled" : "Disabled"));
        }
    }


    void EngineGizmos::DrawBounds(const BoundingBox& aabb)
    {
        if (!Functions::IntersectPlanesAABB(m_frustrumPlanes.array_ptr(), 6, aabb))
        {
            return;
        }
        
        auto idx = m_vertexCount;
        m_vertexCount += 24u;

        if (m_vertexCount > m_maxVertices)
        {
            return;
        }

        auto min = aabb.min;
        auto max = aabb.max;
        m_vertexView[idx++] = { m_matrix * float4(min.x, min.y, min.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(min.x, max.y, min.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(max.x, min.y, min.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(max.x, max.y, min.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(min.x, min.y, max.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(min.x, max.y, max.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(max.x, min.y, max.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(max.x, max.y, max.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(min.x, min.y, min.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(max.x, min.y, min.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(min.x, max.y, min.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(max.x, max.y, min.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(min.x, min.y, max.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(max.x, min.y, max.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(min.x, max.y, max.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(max.x, max.y, max.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(min.x, min.y, min.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(min.x, min.y, max.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(max.x, min.y, min.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(max.x, min.y, max.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(min.x, max.y, min.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(min.x, max.y, max.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(max.x, max.y, min.z, 1.0f), m_color };
        m_vertexView[idx++] = { m_matrix * float4(max.x, max.y, max.z, 1.0f), m_color };
    }

    void EngineGizmos::DrawBox(const float3& origin, const float3& size)
    {
        DrawBounds(BoundingBox::MinMax(origin, origin + size));
    }

    void EngineGizmos::DrawLine(const float3& start, const float3& end)
    {
        auto idx = m_vertexCount;
        m_vertexCount += 2u;

        if (m_vertexCount <= m_maxVertices)
        {
            m_vertexView[idx++] = { m_matrix * float4(start, 1.0f), m_color };
            m_vertexView[idx++] = { m_matrix * float4(end, 1.0f),   m_color };
        }
    }

    void EngineGizmos::DrawRay(const float3& origin, const float3& vector)
    {
        DrawLine(origin, origin + vector);
    }

    void EngineGizmos::DrawFrustrum(const float4x4& matrix)
    {
        auto idx = m_vertexCount;
        m_vertexCount += 24u;

        if (m_vertexCount > m_maxVertices)
        {
            return;
        }

        float3 nearCorners[4];
        float3 farCorners[4];
        auto planes = Functions::ExtractFrustrumPlanes(matrix, true);

        auto temp = planes[1];
        planes[1] = planes[2];
        planes[2] = temp;

        for (auto i = 0; i < 4; ++i)
        {
            nearCorners[i] = Functions::IntesectPlanes3(planes.near, planes[i], planes[(i + 1) % 4]);
            farCorners[i] = Functions::IntesectPlanes3(planes.far, planes[i], planes[(i + 1) % 4]);
        }

        for (auto i = 0; i < 4; ++i)
        {
            m_vertexView[idx++] = { m_matrix * float4(nearCorners[i], 1.0f),           m_color };
            m_vertexView[idx++] = { m_matrix * float4(nearCorners[(i + 1) % 4], 1.0f), m_color };
            m_vertexView[idx++] = { m_matrix * float4(farCorners[i], 1.0f),            m_color };
            m_vertexView[idx++] = { m_matrix * float4(farCorners[(i + 1) % 4], 1.0f),  m_color };
            m_vertexView[idx++] = { m_matrix * float4(nearCorners[i], 1.0f),           m_color };
            m_vertexView[idx++] = { m_matrix * float4(farCorners[i], 1.0f),            m_color };
        }
    }

    void EngineGizmos::SetColor(const color& color)
    {
        m_color = color * 255.0f;
    }

    void EngineGizmos::SetMatrix(const float4x4& matrix)
    {
        auto vp = m_ViewToClip * matrix;
        m_frustrumPlanes = Math::Functions::ExtractFrustrumPlanes(vp, true);
        m_matrix = matrix;
    }
}
