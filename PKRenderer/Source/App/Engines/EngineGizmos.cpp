#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsIntersect.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/IRenderPipeline.h"
#include "App/Renderer/HashCache.h"
#include "App/RendererConfig.h"
#include "EngineGizmos.h"

namespace PK::App
{
    EngineGizmos::EngineGizmos(AssetDatabase* assetDatabase, Sequencer* sequencer, RendererConfig* config) :
        m_sequencer(sequencer)
    {
        m_enabledCPU = config->EnableGizmosCPU;
        m_enabledGPU = config->EnableGizmosGPU;
        m_gizmosShader = assetDatabase->Find<ShaderAsset>("VS_Gizmos");
        m_vertexBuffer = RHI::CreateBuffer<uint4>(m_maxVertices, BufferUsage::DefaultVertex | BufferUsage::PersistentStage, "Gizmos.VertexBuffer");
        m_indirectVertexBuffer = RHI::CreateBuffer<uint4>(16384u, BufferUsage::Vertex | BufferUsage::Storage, "Gizmos.Indirect.VertexBuffer");
        m_indirectArgsBuffer = RHI::CreateBuffer<uint4>(1u, BufferUsage::Storage | BufferUsage::Indirect | BufferUsage::TransferDst, "Gizmos.Indirect.Arguments");
        m_fixedFunctionAttribs = m_gizmosShader->GetFixedFunctionAttributes();
        m_fixedFunctionAttribs.rasterization.polygonMode = PolygonMode::Line;
        m_fixedFunctionAttribs.rasterization.topology = Topology::LineList;

        m_vertexStreamElement.name = PK_RHI_VS_POSITION;
        m_vertexStreamElement.stream = 0u;
        m_vertexStreamElement.inputRate = InputRate::PerVertex;
        m_vertexStreamElement.stride = sizeof(uint4);
        m_vertexStreamElement.offset = 0u;
        m_vertexStreamElement.size = sizeof(uint4);

        auto hash = HashCache::Get();
        RHI::SetBuffer(hash->pk_Gizmos_IndirectVertices, m_indirectVertexBuffer.get());
        RHI::SetBuffer(hash->pk_Gizmos_IndirectArguments, m_indirectArgsBuffer.get());

        CVariableRegister::Create<bool*>("Engine.Gizmos.CPU.Enabled", &m_enabledCPU, "0 = 0ff, 1 = On", 1u, 1u);
        CVariableRegister::Create<bool*>("Engine.Gizmos.GPU.Enabled", &m_enabledGPU, "0 = 0ff, 1 = On", 1u, 1u);
        CVariableRegister::Create<CVariableFuncSimple>("Engine.Gizmos.CPU.Toggle", [this](){m_enabledCPU ^= true;});
        CVariableRegister::Create<CVariableFuncSimple>("Engine.Gizmos.GPU.Toggle", [this](){m_enabledGPU ^= true;});
    }

    void EngineGizmos::Step(RenderPipelineEvent* renderEvent)
    {
        auto view = renderEvent->context->views[0];
        auto gbuffers = view->GetGBuffersFullView();

        switch (renderEvent->type)
        {
            case RenderPipelineEvent::CollectDraws:
            {
                if (m_enabledCPU)
                {
                    m_vertexBuffer->Validate<uint4>(m_vertexCount);
                    m_color = PK_COLOR_WHITE;
                    m_matrix = PK_FLOAT4X4_IDENTITY;
                    m_worldToClip = view->worldToClip;
                    m_vertexCount = 0u;
                    m_maxVertices = (uint32_t)m_vertexBuffer->GetCount<uint4>();
                    m_vertexView = renderEvent->cmd.BeginBufferWrite<Vertex>(m_vertexBuffer.get());
                    m_sequencer->Next<IGizmos*>(this, this);
                    renderEvent->cmd->EndBufferWrite(m_vertexBuffer.get());
                }

                uint4 clearValue{ 0u, 1u, 0u, 0u };
                renderEvent->cmd->UpdateBuffer(m_indirectArgsBuffer.get(), 0u, sizeof(uint4), &clearValue);
            }
            return;
            case RenderPipelineEvent::AfterPostEffects:
            {
                if (m_enabledGPU)
                {
                    auto rect = gbuffers.current.color->GetRect();
                    const RHIBuffer* vb = m_indirectVertexBuffer.get();
                    renderEvent->cmd->SetVertexBuffers(&vb, 1u);
                    renderEvent->cmd->SetVertexStreams(&m_vertexStreamElement, 1u);
                    renderEvent->cmd.SetShader(m_gizmosShader);
                    renderEvent->cmd.SetRenderTarget(gbuffers.current.color);
                    renderEvent->cmd.SetViewPort(rect);
                    renderEvent->cmd.SetScissor(rect);
                    renderEvent->cmd.SetFixedStateAttributes(&m_fixedFunctionAttribs);
                    renderEvent->cmd->DrawIndirect(m_indirectArgsBuffer.get(), 0u, 1u, sizeof(uint4));
                }

                if (m_enabledCPU && m_vertexCount >= 2)
                {
                    auto rect = gbuffers.current.color->GetRect();
                    const RHIBuffer* vb = m_vertexBuffer.get();
                    renderEvent->cmd->SetVertexBuffers(&vb, 1u);
                    renderEvent->cmd->SetVertexStreams(&m_vertexStreamElement, 1u);
                    renderEvent->cmd.SetShader(m_gizmosShader);
                    renderEvent->cmd.SetRenderTarget(gbuffers.current.color);
                    renderEvent->cmd.SetViewPort(rect);
                    renderEvent->cmd.SetScissor(rect);
                    renderEvent->cmd.SetFixedStateAttributes(&m_fixedFunctionAttribs);
                    renderEvent->cmd->Draw(glm::min(m_vertexCount, m_maxVertices), 1u, 0u, 0u);
                }
            }
            return;

            default: return;
        }
    }

    void EngineGizmos::Step(AssetImportEvent<RendererConfig>* token)
    {
        m_enabledCPU = token->asset->EnableGizmosCPU;
        m_enabledGPU = token->asset->EnableGizmosGPU;
    }

    void EngineGizmos::DrawBounds(const BoundingBox& aabb)
    {
        if (!Math::IntersectPlanesAABB(m_frustrumPlanes.array_ptr(), 6, aabb))
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
        auto planes = Math::ExtractFrustrumPlanes(matrix, true);

        auto temp = planes[1];
        planes[1] = planes[2];
        planes[2] = temp;

        for (auto i = 0; i < 4; ++i)
        {
            nearCorners[i] = Math::IntesectPlanes3(planes.near, planes[i], planes[(i + 1) % 4]);
            farCorners[i] = Math::IntesectPlanes3(planes.far, planes[i], planes[(i + 1) % 4]);
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
        auto vp = m_worldToClip * matrix;
        m_frustrumPlanes = Math::ExtractFrustrumPlanes(vp, true);
        m_matrix = matrix;
    }
}
