#include "PrecompiledHeader.h"
#include "Core/Math/Extended.h"
#include "Core/Math/Bounds.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "App/Renderer/RenderPipelineBase.h"
#include "App/Renderer/HashCache.h"
#include "EngineGUIRenderer.h"

namespace PK::App
{
    EngineGUIRenderer::EngineGUIRenderer(AssetDatabase* assetDatabase, Sequencer* sequencer) : 
        m_sequencer(sequencer),
        m_assetDatabase(assetDatabase)
    { 
        m_gizmos_shader = assetDatabase->Find<ShaderAsset>("VS_Gizmos").get();
        m_gizmos_vertexBuffer = RHI::CreateBuffer<uint4>(m_gizmos_maxVertices, BufferUsage::DefaultVertex | BufferUsage::PersistentStage, "Gizmos.VertexBuffer");
        m_gizmos_indirectVertexBuffer = RHI::CreateBuffer<uint4>(16384u, BufferUsage::Vertex | BufferUsage::Storage, "Gizmos.Indirect.VertexBuffer");
        m_gizmos_indirectArgsBuffer = RHI::CreateBuffer<uint4>(1u, BufferUsage::Storage | BufferUsage::Indirect | BufferUsage::TransferDst, "Gizmos.Indirect.Arguments");
        m_gizmos_fixedFunctionAttribs = m_gizmos_shader->GetFixedFunctionAttributes();
        m_gizmos_fixedFunctionAttribs.rasterization.polygonMode = PolygonMode::Line;
        m_gizmos_fixedFunctionAttribs.rasterization.topology = Topology::LineList;

        m_gizmos_vertexStreamElement.name = PK_RHI_VS_POSITION;
        m_gizmos_vertexStreamElement.stream = 0u;
        m_gizmos_vertexStreamElement.inputRate = InputRate::PerVertex;
        m_gizmos_vertexStreamElement.stride = sizeof(uint4);
        m_gizmos_vertexStreamElement.offset = 0u;
        m_gizmos_vertexStreamElement.format = ElementType::Uint4;

        auto hash = HashCache::Get();
        RHI::SetBuffer(hash->pk_Gizmos_IndirectVertices, m_gizmos_indirectVertexBuffer.get());
        RHI::SetBuffer(hash->pk_Gizmos_IndirectArguments, m_gizmos_indirectArgsBuffer.get());

        CVariableRegister::Create<bool*>("Engine.GUI.Enabled", &m_gui_enabled, "0 = 0ff, 1 = On", 1u);
        CVariableRegister::Create<bool*>("Engine.Gizmos.CPU.Enabled", &m_gizmos_enabledCPU, "0 = 0ff, 1 = On", 1u);
        CVariableRegister::Create<bool*>("Engine.Gizmos.GPU.Enabled", &m_gizmos_enabledGPU, "0 = 0ff, 1 = On", 1u);
        CVariableRegister::Create<CVariableFuncSimple>("Engine.Gizmos.CPU.Toggle", [this]() { m_gizmos_enabledCPU ^= true; });
        CVariableRegister::Create<CVariableFuncSimple>("Engine.Gizmos.GPU.Toggle", [this]() { m_gizmos_enabledGPU ^= true; });
    }

    void EngineGUIRenderer::Step(RenderPipelineEvent* renderEvent)
    {
        auto view = renderEvent->context->views[0];

        switch (renderEvent->type)
        {
            case RenderPipelineEvent::CollectDraws:
            {
                GUICollectDraws(view->renderAreaRect, renderEvent->cmd);
                GizmosCollectDraws(view->renderAreaRect, view->worldToClip, renderEvent->cmd);
            }
            return;
            case RenderPipelineEvent::AfterPostEffects:
            {
                auto gbuffers = view->GetGBuffersFullView();
                GizmosDispatchDraws(renderEvent->cmd, gbuffers.current.color);
                GUIDispatchDraws(renderEvent->cmd, gbuffers.current.color);
            }
            return;

            default: return;
        }
    }


    void EngineGUIRenderer::GUICollectDraws(const uint4& renderArea, CommandBufferExt& cmd)
    {
        m_gui_vertexCount = 0u;
        m_gui_indexCount = 0u;
        m_gui_renderAreaRect = renderArea;
        m_gui_vertexView = {};
        m_gui_indexView = {};
        m_gui_commandBuffer = m_gui_enabled ? &cmd : nullptr;

        if (m_gui_enabled)
        {
            GUIDrawList drawList(this);
            m_sequencer->Next<GUIDrawList*>(this, &drawList);
        }

        if (m_gui_vertexView.data != nullptr)
        {
            cmd->EndBufferWrite(m_gui_vertexBuffer.get());
            cmd->EndBufferWrite(m_gui_indexBuffer.get());
        }

        m_gui_commandBuffer = nullptr;
    }

    void EngineGUIRenderer::GUIDispatchDraws(CommandBufferExt& cmd, RHITexture* target)
    {
        if (m_gui_vertexCount >= 2)
        {
            RHI::SetTextureSet(HashCache::Get()->pk_GUI_Textures, m_gui_textures.get());
            cmd->SetIndexBuffer(m_gui_indexBuffer.get(), sizeof(uint16_t));
            cmd.SetShader(m_gui_shader);
            cmd.SetRenderTarget({ target, LoadOp::Load, StoreOp::Store }, true);
            cmd->DrawIndexed(math::min(GUI_MAX_INDICES, m_gui_indexCount), 1u, 0u, 0u, 0u);
        }
    }

    bool EngineGUIRenderer::GUIValidateDraw()
    {
        if (m_gui_commandBuffer)
        {
            // initialize and load resources if the dont exist.
            if (m_gui_shader == nullptr)
            {
                m_gui_shader = m_assetDatabase->Find<ShaderAsset>("VS_GUI").get();
                m_gui_font = m_assetDatabase->Load<Font>("Content/Fonts/FSEX302.pkfont").get();
                m_gui_vertexBuffer = RHI::CreateBuffer<GUIVertex>(GUI_MAX_VERTICES, BufferUsage::PersistentStorage, "GUI.VertexBuffer");
                m_gui_indexBuffer = RHI::CreateBuffer<GUIIndex>(GUI_MAX_INDICES, BufferUsage::DefaultIndex | BufferUsage::PersistentStage, "GUI.IndexBuffer");
                m_gui_textures = RHI::CreateBindSet<RHITexture>(GUI_MAX_TEXTURES);
                RHI::SetBuffer(HashCache::Get()->pk_GUI_Vertices, m_gui_vertexBuffer.get());
            }

            // Initialize draw state
            if (m_gui_vertexView.data == nullptr)
            {
                m_gui_vertexView = m_gui_commandBuffer->BeginBufferWrite<GUIVertex>(m_gui_vertexBuffer.get());
                m_gui_indexView = m_gui_commandBuffer->BeginBufferWrite<uint16_t>(m_gui_indexBuffer.get());
                m_gui_textures->Clear();
                m_gui_textures->Add(RHI::GetBuiltInResources()->WhiteTexture2D.get());
                m_gui_textures->Add(RHI::GetBuiltInResources()->ErrorTexture2D.get());
                m_gui_textures->Add(m_gui_font->GetRHI());
            }
        }
        
        return m_gui_commandBuffer != nullptr;
    }

    short4 EngineGUIRenderer::GUIGetRenderAreaRect() const
    {
        return m_gui_renderAreaRect;
    }

    Font* EngineGUIRenderer::GUIGetDefaultFont() const
    {
        return m_gui_font;
    }

    uint16_t EngineGUIRenderer::GUIGetTextureIndex(RHITexture* texture)
    {
        if (!GUIValidateDraw())
        {
            return GUI_TEX_INDEX_ERROR;
        }

        if (texture == nullptr || texture->IsTracked())
        {
            PK_LOG_WARNING("Trying to add null or non readonly texture to be drawn in gui!");
            // Return error tex handle
            return GUI_TEX_INDEX_ERROR;
        }

        return (uint16_t)m_gui_textures->Add(texture);
    }

    uint3 EngineGUIRenderer::GUIGetTextureSize(uint16_t textureIndex) const
    {
        return m_gui_textures->GetBoundTextureSize(textureIndex);
    }

    bool EngineGUIRenderer::GUIAllocate(uint32_t layer, uint32_t vertexCount, uint32_t indexCount, GUIAllocation* allocation)
    {
        *allocation = {};

        if (GUIValidateDraw() &&
            m_gui_vertexCount + vertexCount <= GUI_MAX_VERTICES &&
            m_gui_indexCount + indexCount <= GUI_MAX_INDICES)
        {
            allocation->vertices = m_gui_vertexView.data + m_gui_vertexCount;
            allocation->indices = m_gui_indexView.data + m_gui_indexCount;
            allocation->vertexOffset = m_gui_vertexCount;
            allocation->vertexCount = vertexCount;
            allocation->indexCount = indexCount;
            allocation->layer = layer;
            m_gui_vertexCount += vertexCount;
            m_gui_indexCount += indexCount;
            return true;
        }

        return false;
    }

 
    void EngineGUIRenderer::GizmosCollectDraws(const uint4& renderArea, const float4x4& worldToClip, CommandBufferExt& cmd)
    {
        if (m_gizmos_enabledCPU)
        {
            RHI::ValidateBuffer<uint4>(m_gizmos_vertexBuffer, m_gizmos_vertexCount);
            m_gizmos_color = PK_COLOR_WHITE;
            m_gizmos_renderAreaRect = renderArea;
            m_gizmos_matrix = PK_FLOAT4X4_IDENTITY;
            m_gizmos_worldToClip = worldToClip;
            m_gizmos_vertexCount = 0u;
            m_gizmos_maxVertices = (uint32_t)m_gizmos_vertexBuffer->GetCount<uint4>();
            m_gizmos_vertexView = cmd.BeginBufferWrite<GizmosVertex>(m_gizmos_vertexBuffer.get());
            m_sequencer->Next<IGizmosRenderer*>(this, this);
            cmd->EndBufferWrite(m_gizmos_vertexBuffer.get());
        }

        uint4 clearValue{ 0u, 1u, 0u, 0u };
        cmd->UpdateBuffer(m_gizmos_indirectArgsBuffer.get(), 0u, sizeof(uint4), &clearValue);
    }

    void EngineGUIRenderer::GizmosDispatchDraws(CommandBufferExt& cmd, RHITexture* target)
    {
        if (m_gizmos_enabledGPU)
        {
            const RHIBuffer* vb = m_gizmos_indirectVertexBuffer.get();
            cmd->SetVertexBuffers(&vb, 1u);
            cmd->SetVertexStreams(&m_gizmos_vertexStreamElement, 1u);
            cmd.SetShader(m_gizmos_shader);
            cmd.SetRenderTarget({ target, LoadOp::Load, StoreOp::Store }, true);
            cmd.SetFixedStateAttributes(&m_gizmos_fixedFunctionAttribs);
            cmd->DrawIndirect(m_gizmos_indirectArgsBuffer.get(), 0u, 1u, sizeof(uint4));
        }

        if (m_gizmos_enabledCPU && m_gizmos_vertexCount >= 2)
        {
            const RHIBuffer* vb = m_gizmos_vertexBuffer.get();
            cmd->SetVertexBuffers(&vb, 1u);
            cmd->SetVertexStreams(&m_gizmos_vertexStreamElement, 1u);
            cmd.SetShader(m_gizmos_shader);
            cmd.SetRenderTarget({ target, LoadOp::Load, StoreOp::Store }, true);
            cmd.SetFixedStateAttributes(&m_gizmos_fixedFunctionAttribs);
            cmd->Draw(math::min(m_gizmos_vertexCount, m_gizmos_maxVertices), 1u, 0u, 0u);
        }
    }

    void EngineGUIRenderer::GizmosDrawBounds(const AABB<float3>& aabb)
    {
        if (m_gizmos_enabledCPU && math::intersectsConvex(aabb, m_gizmos_frustrumPlanes.array_ptr(), 6))
        {
            auto idx = m_gizmos_vertexCount;
            m_gizmos_vertexCount += 24u;

            if (m_gizmos_vertexCount <= m_gizmos_maxVertices)
            {
                auto min = &aabb.min.x;
                auto max = &aabb.max.x;

                for (auto i = 0u; i < 4u; ++i)
                {
                    m_gizmos_vertexView[idx++] = { float3(m_gizmos_matrix * float4(min[0], i & 1u ? min[1] : max[1], i & 2u ? min[2] : max[2], 1.0f)), m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { float3(m_gizmos_matrix * float4(max[0], i & 1u ? min[1] : max[1], i & 2u ? min[2] : max[2], 1.0f)), m_gizmos_color };
                }

                for (auto i = 0u; i < 4u; ++i)
                {
                    m_gizmos_vertexView[idx++] = { float3(m_gizmos_matrix * float4(i & 1u ? min[0] : max[0], min[1], i & 2u ? min[2] : max[2], 1.0f)), m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { float3(m_gizmos_matrix * float4(i & 1u ? min[0] : max[0], max[1], i & 2u ? min[2] : max[2], 1.0f)), m_gizmos_color };
                }

                for (auto i = 0u; i < 4u; ++i)
                {
                    m_gizmos_vertexView[idx++] = { float3(m_gizmos_matrix * float4(i & 1u ? min[0] : max[0], i & 2u ? min[1] : max[1], min[2], 1.0f)), m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { float3(m_gizmos_matrix * float4(i & 1u ? min[0] : max[0], i & 2u ? min[1] : max[1], max[2], 1.0f)), m_gizmos_color };
                }
            }
        }
    }

    void EngineGUIRenderer::GizmosDrawBox(const float3& origin, const float3& size)
    {
        GizmosDrawBounds(AABB<float3>(origin, origin + size));
    }

    void EngineGUIRenderer::GizmosDrawLine(const float3& start, const float3& end)
    {
        if (m_gizmos_enabledCPU)
        {
            auto idx = m_gizmos_vertexCount;
            m_gizmos_vertexCount += 2u;

            if (m_gizmos_vertexCount <= m_gizmos_maxVertices)
            {
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(start, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(end, 1.0f)).xyz,   m_gizmos_color };
            }
        }
    }

    void EngineGUIRenderer::GizmosDrawRay(const float3& origin, const float3& vector)
    {
        GizmosDrawLine(origin, origin + vector);
    }

    void EngineGUIRenderer::GizmosDrawFrustrum(const float4x4& matrix)
    {
        if (m_gizmos_enabledCPU)
        {
            auto idx = m_gizmos_vertexCount;
            m_gizmos_vertexCount += 24u;

            if (m_gizmos_vertexCount <= m_gizmos_maxVertices)
            {
                float3 nearCorners[4];
                float3 farCorners[4];
                auto planes = FrustumPlanes(math::frustumConvex<true>(matrix));

                auto temp = planes[1];
                planes[1] = planes[2];
                planes[2] = temp;

                for (auto i = 0u; i < 4u; ++i)
                {
                    nearCorners[i] = math::triplanarIntersection(planes.near(), planes[i], planes[(i + 1u) % 4u]);
                    farCorners[i] = math::triplanarIntersection(planes.far(), planes[i], planes[(i + 1u) % 4u]);
                }

                for (auto i = 0; i < 4; ++i)
                {
                    m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(nearCorners[i], 1.0f)).xyz,           m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(nearCorners[(i + 1) % 4], 1.0f)).xyz, m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(farCorners[i], 1.0f)).xyz,            m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(farCorners[(i + 1) % 4], 1.0f)).xyz,  m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(nearCorners[i], 1.0f)).xyz,           m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(farCorners[i], 1.0f)).xyz,            m_gizmos_color };
                }
            }
        }
    }

    void EngineGUIRenderer::GizmosSetColor(const color& color)
    {
        m_gizmos_color = color * 255.0f;
    }

    void EngineGUIRenderer::GizmosSetMatrix(const float4x4& matrix)
    {
        auto vp = m_gizmos_worldToClip * matrix;
        m_gizmos_frustrumPlanes = math::frustumConvex<true>(vp);
        m_gizmos_matrix = matrix;
    }

    const float4x4& EngineGUIRenderer::GizmosGetWorldToClipMatrix() const
    {
        return m_gizmos_worldToClip;
    }

    const short4& EngineGUIRenderer::GizmosGetRenderAreaRect() const
    {
        return m_gizmos_renderAreaRect;
    }
}
