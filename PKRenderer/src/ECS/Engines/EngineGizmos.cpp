#include "PrecompiledHeader.h"
#include "EngineGizmos.h"
#include "Math/FunctionsIntersect.h"

namespace PK::ECS::Engines
{
    using namespace Math;
    using namespace Core;
    using namespace Core::Services;
    using namespace Rendering::Objects;
    using namespace Rendering::Structs;

    EngineGizmos::EngineGizmos(AssetDatabase* assetDatabase, Sequencer* sequencer, ApplicationConfig* config) :
        m_sequencer(sequencer)
    {
        m_enabled = config->EnableGizmos;
        m_gizmosShader = assetDatabase->Find<Shader>("SH_VS_Gizmos");
        m_vertexBuffer = Buffer::Create(BufferLayout({ { ElementType::Uint4, "in_POSITION" } }), m_maxVertices, BufferUsage::DefaultVertex | BufferUsage::PersistentStage, "Gizmos.VertexBuffer");
        m_fixedFunctionAttribs = m_gizmosShader->GetFixedFunctionAttributes();
        m_fixedFunctionAttribs.rasterization.polygonMode = PolygonMode::Line;
        m_fixedFunctionAttribs.rasterization.topology = Topology::LineList;
    }

    void EngineGizmos::Step(Tokens::TokenRenderCollectDrawCalls* token)
    {
        if (!m_enabled)
        {
            return;
        }

        m_vertexBuffer->Validate(m_vertexCount);
        m_color = Math::PK_COLOR_WHITE;
        m_matrix = Math::PK_FLOAT4X4_IDENTITY;
        m_viewprojection = token->viewProjection;
        m_vertexCount = 0u;
        m_maxVertices = (uint32_t)m_vertexBuffer->GetCount();
        m_vertexView = token->cmd->BeginBufferWrite<Vertex>(m_vertexBuffer.get());
        m_sequencer->Next<IGizmosRenderer>(this, this, 0);
        token->cmd->EndBufferWrite(m_vertexBuffer.get());
    }

    void EngineGizmos::Step(Tokens::TokenRenderAfterPostEffects* token)
    {
        if (m_vertexCount < 2 || !m_enabled)
        {
            return;
        }
     
        auto vertexCount = glm::min(m_vertexCount, m_maxVertices);
        auto rect = token->renderTarget->GetRect();
        const Buffer* vb = m_vertexBuffer.get();
        const Buffer** vbptr = &vb;

        token->cmd->SetVertexBuffers(vbptr, 1u);
        token->cmd->SetShader(m_gizmosShader);
        token->cmd->SetRenderTarget(token->renderTarget->GetColor(0));
        token->cmd->SetViewPort(rect);
        token->cmd->SetScissor(rect);
        token->cmd->SetFixedStateAttributes(&m_fixedFunctionAttribs);
        token->cmd->Draw(vertexCount, 1u, 0u, 0u);
    }

    void EngineGizmos::Step(Core::Services::AssetImportToken<Core::ApplicationConfig>* token)
    {
        m_enabled = token->asset->EnableGizmos;
    }

    void EngineGizmos::Step(Core::TokenConsoleCommand* token)
    {
        if (!token->isConsumed && token->argument == "toggle_gizmos")
        {
            m_enabled ^= true;
            token->isConsumed = true;
            PK_LOG_INFO("Gizmos %s", (m_enabled ? "Enabled" : "Disabled"));
        }
    }


    void EngineGizmos::DrawBounds(const BoundingBox& aabb)
    {
        if (!Functions::IntersectPlanesAABB(m_frustrumPlanes.planes, 6, aabb))
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
        FrustumPlanes frustum;

        Functions::ExtractFrustrumPlanes(matrix, &frustum, true);

        auto temp = frustum.planes[1];
        frustum.planes[1] = frustum.planes[2];
        frustum.planes[2] = temp;

        for (auto i = 0; i < 4; ++i)
        {
            nearCorners[i] = Functions::IntesectPlanes3(frustum.planes[4], frustum.planes[i], frustum.planes[(i + 1) % 4]);
            farCorners[i] = Functions::IntesectPlanes3(frustum.planes[5], frustum.planes[i], frustum.planes[(i + 1) % 4]);
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
        auto vp = m_viewprojection * matrix;
        Math::Functions::ExtractFrustrumPlanes(vp, &m_frustrumPlanes, true);
        m_matrix = matrix;
    }
}