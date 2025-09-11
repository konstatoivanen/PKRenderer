#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Math/FunctionsIntersect.h"
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
        m_gui_textures(GUI_MAX_TEXTURES)
    {
        auto hash = HashCache::Get();

        m_gui_shader = assetDatabase->Find<ShaderAsset>("VS_GUI");
        m_gui_font = assetDatabase->Load<Font>("Content/Fonts/FSEX302.pkfont");
        m_gui_vertexBuffer = RHI::CreateBuffer<GUIVertex>(GUI_MAX_VERTICES, BufferUsage::PersistentStorage, "GUI.VertexBuffer");
        m_gui_indexBuffer = RHI::CreateBuffer<uint16_t>(GUI_MAX_INDICES, BufferUsage::DefaultIndex | BufferUsage::PersistentStage, "GUI.IndexBuffer");

        SamplerDescriptor samplerDesc{};
        samplerDesc.anisotropy = 1.0f;
        samplerDesc.filterMin = FilterMode::Bilinear;
        samplerDesc.filterMag = FilterMode::Bilinear;
        samplerDesc.wrap[0] = WrapMode::Repeat;
        samplerDesc.wrap[1] = WrapMode::Repeat;
        samplerDesc.wrap[2] = WrapMode::Repeat;
        samplerDesc.normalized = true;
        RHI::SetSampler(hash->pk_Sampler_GUI, samplerDesc);
        RHI::SetBuffer(hash->pk_GUI_Vertices, m_gui_vertexBuffer.get());

        CVariableRegister::Create<bool*>("Engine.GUI.Enabled", &m_gui_enabled, "0 = 0ff, 1 = On", 1u);
        CVariableRegister::Create<CVariableFuncSimple>("Engine.GUI.Toggle", [this]() { m_gui_enabled^= true; });
    
        m_gizmos_shader = assetDatabase->Find<ShaderAsset>("VS_Gizmos");
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
        m_gizmos_vertexStreamElement.size = sizeof(uint4);

        RHI::SetBuffer(hash->pk_Gizmos_IndirectVertices, m_gizmos_indirectVertexBuffer.get());
        RHI::SetBuffer(hash->pk_Gizmos_IndirectArguments, m_gizmos_indirectArgsBuffer.get());

        CVariableRegister::Create<bool*>("Engine.Gizmos.CPU.Enabled", &m_gizmos_enabledCPU, "0 = 0ff, 1 = On", 1u);
        CVariableRegister::Create<bool*>("Engine.Gizmos.GPU.Enabled", &m_gizmos_enabledGPU, "0 = 0ff, 1 = On", 1u);
        CVariableRegister::Create<CVariableFuncSimple>("Engine.Gizmos.CPU.Toggle", [this]() { m_gizmos_enabledCPU ^= true; });
        CVariableRegister::Create<CVariableFuncSimple>("Engine.Gizmos.GPU.Toggle", [this]() { m_gizmos_enabledGPU ^= true; });
    }

    void EngineGUIRenderer::Step(RenderPipelineEvent* renderEvent)
    {
        auto hash = HashCache::Get();
        auto view = renderEvent->context->views[0];
        auto gbuffers = view->GetGBuffersFullView();

        switch (renderEvent->type)
        {
            case RenderPipelineEvent::CollectDraws:
            {
                if (m_gizmos_enabledCPU)
                {
                    RHI::ValidateBuffer<uint4>(m_gizmos_vertexBuffer, m_gizmos_vertexCount);
                    m_gizmos_color = PK_COLOR_WHITE;
                    m_gizmos_matrix = PK_FLOAT4X4_IDENTITY;
                    m_gizmos_worldToClip = view->worldToClip;
                    m_gizmos_vertexCount = 0u;
                    m_gizmos_maxVertices = (uint32_t)m_gizmos_vertexBuffer->GetCount<uint4>();
                    m_gizmos_vertexView = renderEvent->cmd.BeginBufferWrite<GizmosVertex>(m_gizmos_vertexBuffer.get());
                }

                if (m_gui_enabled)
                {
                    m_gui_vertexCount = 0u;
                    m_gui_indexCount = 0u;
                    m_gui_renderAreaRect = view->renderAreaRect;
                    m_gui_vertexView = renderEvent->cmd.BeginBufferWrite<GUIVertex>(m_gui_vertexBuffer.get());
                    m_gui_indexView = renderEvent->cmd.BeginBufferWrite<uint16_t>(m_gui_indexBuffer.get());
                    m_gui_textures.Clear();
                    m_gui_textures.Set(RHI::GetBuiltInResources()->WhiteTexture2D.get());
                    m_gui_textures.Set(RHI::GetBuiltInResources()->ErrorTexture2D.get());
                    m_gui_textures.Set(m_gui_font->GetRHI());
                }

                if (m_gizmos_enabledCPU)
                {
                    m_sequencer->Next<IGizmosRenderer*>(this, this);
                }

                if (m_gui_enabled)
                {
                    m_sequencer->Next<IGUIRenderer*>(this, this);
                }

                if (m_gui_enabled || m_gizmos_enabledCPU)
                {
                    GUICombinedRenderEvent guiEvent;
                    guiEvent.gizmos = this;
                    guiEvent.gui = this;
                    m_sequencer->Next<GUICombinedRenderEvent>(this, guiEvent);
                }

                if (m_gizmos_enabledCPU)
                {
                    renderEvent->cmd->EndBufferWrite(m_gizmos_vertexBuffer.get());
                }

                if (m_gui_enabled)
                {
                    renderEvent->cmd->EndBufferWrite(m_gui_vertexBuffer.get());
                    renderEvent->cmd->EndBufferWrite(m_gui_indexBuffer.get());
                }

                uint4 clearValue{ 0u, 1u, 0u, 0u };
                renderEvent->cmd->UpdateBuffer(m_gizmos_indirectArgsBuffer.get(), 0u, sizeof(uint4), &clearValue);
            }
            return;
            case RenderPipelineEvent::AfterPostEffects:
            {
                if (m_gizmos_enabledGPU)
                {
                    const RHIBuffer* vb = m_gizmos_indirectVertexBuffer.get();
                    renderEvent->cmd->SetVertexBuffers(&vb, 1u);
                    renderEvent->cmd->SetVertexStreams(&m_gizmos_vertexStreamElement, 1u);
                    renderEvent->cmd.SetShader(m_gizmos_shader);
                    renderEvent->cmd.SetRenderTarget({ gbuffers.current.color, LoadOp::Load, StoreOp::Store }, true);
                    renderEvent->cmd.SetFixedStateAttributes(&m_gizmos_fixedFunctionAttribs);
                    renderEvent->cmd->DrawIndirect(m_gizmos_indirectArgsBuffer.get(), 0u, 1u, sizeof(uint4));
                }

                if (m_gizmos_enabledCPU && m_gizmos_vertexCount >= 2)
                {
                    const RHIBuffer* vb = m_gizmos_vertexBuffer.get();
                    renderEvent->cmd->SetVertexBuffers(&vb, 1u);
                    renderEvent->cmd->SetVertexStreams(&m_gizmos_vertexStreamElement, 1u);
                    renderEvent->cmd.SetShader(m_gizmos_shader);
                    renderEvent->cmd.SetRenderTarget({ gbuffers.current.color, LoadOp::Load, StoreOp::Store }, true);
                    renderEvent->cmd.SetFixedStateAttributes(&m_gizmos_fixedFunctionAttribs);
                    renderEvent->cmd->Draw(glm::min(m_gizmos_vertexCount, m_gizmos_maxVertices), 1u, 0u, 0u);
                }

                if (m_gui_enabled && m_gui_vertexCount >= 2)
                {
                    RHI::SetTextureArray(hash->pk_GUI_Textures, m_gui_textures);
                    renderEvent->cmd->SetIndexBuffer(m_gui_indexBuffer.get(), ElementType::Ushort);
                    renderEvent->cmd.SetShader(m_gui_shader);
                    renderEvent->cmd.SetRenderTarget({ gbuffers.current.color, LoadOp::Load, StoreOp::Store }, true);
                    renderEvent->cmd->DrawIndexed(glm::min(GUI_MAX_INDICES, m_gui_indexCount), 1u, 0u, 0u, 0u);
                }
            }
            return;

            default: return;
        }
    }


    uint16_t EngineGUIRenderer::GUIAddTexture(RHITexture* texture)
    {
        if (!m_gui_enabled)
        {
            return GUI_TEX_INDEX_ERROR;
        }

        if (texture == nullptr || texture->IsTracked())
        {
            PK_LOG_WARNING("Trying to add null or non readonly texture to be drawn in gui!");
            // Return error tex handle
            return GUI_TEX_INDEX_ERROR;
        }

        return (uint16_t)m_gui_textures.Set(texture);
    }

    void EngineGUIRenderer::GUIDrawTriangle(const GUIVertex& a, const GUIVertex& b, const GUIVertex& c)
    {
        if (m_gui_enabled)
        {
            auto idxv = m_gui_vertexCount;
            auto idxi = m_gui_indexCount;
            m_gui_vertexCount += 3u;
            m_gui_indexCount += 3u;

            if (m_gui_vertexCount <= GUI_MAX_VERTICES && m_gui_indexCount <= GUI_MAX_INDICES)
            {
                m_gui_indexView[idxi++] = idxv + 0;
                m_gui_indexView[idxi++] = idxv + 1;
                m_gui_indexView[idxi++] = idxv + 2;
                m_gui_vertexView[idxv++] = a;
                m_gui_vertexView[idxv++] = b;
                m_gui_vertexView[idxv++] = c;
            }
        }
    }

    void EngineGUIRenderer::GUIDrawRect(const color32& color, const short4& rect, const ushort4& textureRect, RHITexture* texture)
    {
        GUIDrawRect(color, rect, textureRect, GUIAddTexture(texture));
    }

    void EngineGUIRenderer::GUIDrawRect(const color32& color, const short4& rect, const ushort4& textureRect, uint16_t textureIndex)
    {
        if (m_gui_enabled)
        {
            auto idxv = m_gui_vertexCount;
            auto idxi = m_gui_indexCount;
            m_gui_vertexCount += 4u;
            m_gui_indexCount += 6u;

            if (m_gui_vertexCount <= GUI_MAX_VERTICES && m_gui_indexCount <= GUI_MAX_INDICES)
            {
                const short4 sminmax = short4(rect.x, rect.y, rect.x + rect.z, rect.y + rect.w);
                const float4 tminmax = float4(textureRect.x, textureRect.y, textureRect.x + textureRect.z, textureRect.y + textureRect.w);
                const float2 texelSize = m_gui_textures.Get(textureIndex)->GetTexelSize().xy;
                m_gui_indexView[idxi++] = idxv + 0;
                m_gui_indexView[idxi++] = idxv + 1;
                m_gui_indexView[idxi++] = idxv + 2;
                m_gui_indexView[idxi++] = idxv + 2;
                m_gui_indexView[idxi++] = idxv + 3;
                m_gui_indexView[idxi++] = idxv + 0;
                m_gui_vertexView[idxv++] = { color, sminmax.xy, Math::PackHalf(tminmax.xy * texelSize), textureIndex, 0u };
                m_gui_vertexView[idxv++] = { color, sminmax.xw, Math::PackHalf(tminmax.xw * texelSize), textureIndex, 0u };
                m_gui_vertexView[idxv++] = { color, sminmax.zw, Math::PackHalf(tminmax.zw * texelSize), textureIndex, 0u };
                m_gui_vertexView[idxv++] = { color, sminmax.zy, Math::PackHalf(tminmax.zy * texelSize), textureIndex, 0u };
            }
        }
    }

    void EngineGUIRenderer::GUIDrawRect(const color32& color, const short4& rect)
    {
        GUIDrawRect(color, rect, PK_SHORT4_ZERO, GUI_TEX_INDEX_WHITE);
    }

    void EngineGUIRenderer::GUIDrawWireRect(const color32& color, const short4& rect, short inset)
    {
        if (m_gui_enabled)
        {
            auto idxv = m_gui_vertexCount;
            auto idxi = m_gui_indexCount;
            m_gui_vertexCount += 8u;
            m_gui_indexCount += 24u;

            if (m_gui_vertexCount <= GUI_MAX_VERTICES && m_gui_indexCount <= GUI_MAX_INDICES)
            {
                const short4 inner = short4(rect.x + inset, rect.y - inset, rect.x + rect.z - inset, rect.y + rect.w + inset);
                const short4 outer = short4(rect.x, rect.y, rect.x + rect.z, rect.y + rect.w);

                for (auto i = 0u; i < 4; ++i)
                {
                    auto base0 = idxv + i * 2u;
                    auto base1 = idxv + ((i + 1u) % 4u) * 2u;

                    m_gui_indexView[idxi++] = base0 + 0;
                    m_gui_indexView[idxi++] = base0 + 1;
                    m_gui_indexView[idxi++] = base1 + 1;

                    m_gui_indexView[idxi++] = base1 + 1;
                    m_gui_indexView[idxi++] = base1 + 0;
                    m_gui_indexView[idxi++] = base0 + 0;
                }

                m_gui_vertexView[idxv++] = { color, outer.xy, PK_USHORT2_ZERO, 0, 0u };
                m_gui_vertexView[idxv++] = { color, inner.xy, PK_USHORT2_ZERO, 0, 0u };

                m_gui_vertexView[idxv++] = { color, outer.xw, PK_USHORT2_ZERO, 0, 0u };
                m_gui_vertexView[idxv++] = { color, inner.xw, PK_USHORT2_ZERO, 0, 0u };
                
                m_gui_vertexView[idxv++] = { color, outer.zw, PK_USHORT2_ZERO, 0, 0u };
                m_gui_vertexView[idxv++] = { color, inner.zw, PK_USHORT2_ZERO, 0, 0u };
                
                m_gui_vertexView[idxv++] = { color, outer.zy, PK_USHORT2_ZERO, 0, 0u };
                m_gui_vertexView[idxv++] = { color, inner.zy, PK_USHORT2_ZERO, 0, 0u };
            }
        }
    }

    void EngineGUIRenderer::GUIDrawLine(const color32& color0, const color32& color1, const short2& p0, const short2& p1, const float width)
    {
        if (m_gui_enabled)
        {
            auto idxv = m_gui_vertexCount;
            auto idxi = m_gui_indexCount;
            m_gui_vertexCount += 4u;
            m_gui_indexCount += 6u;

            if (m_gui_vertexCount <= GUI_MAX_VERTICES && m_gui_indexCount <= GUI_MAX_INDICES)
            {
                const auto p0f = float2(p0.x + 0.5f, p0.y + 0.5f);
                const auto p1f = float2(p1.x + 0.5f, p1.y + 0.5f);
                const auto direction = glm::normalize(p1f - p0f);
                const auto tangent = float2(-direction.y, direction.x);
                const auto offset = glm::normalize(tangent + direction) * 0.5f * width;
 
                m_gui_indexView[idxi++] = idxv + 0;
                m_gui_indexView[idxi++] = idxv + 1;
                m_gui_indexView[idxi++] = idxv + 2;
                m_gui_indexView[idxi++] = idxv + 2;
                m_gui_indexView[idxi++] = idxv + 3;
                m_gui_indexView[idxi++] = idxv + 0;
                m_gui_vertexView[idxv++] = { color0, glm::round(p0f + float2(-offset.y, +offset.x)), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, 0u };
                m_gui_vertexView[idxv++] = { color1, glm::round(p1f + float2(+offset.x, +offset.y)), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, 0u };
                m_gui_vertexView[idxv++] = { color1, glm::round(p1f + float2(+offset.y, -offset.x)), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, 0u };
                m_gui_vertexView[idxv++] = { color0, glm::round(p0f + float2(-offset.x, -offset.y)), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, 0u };
            }
        }
    }

    void EngineGUIRenderer::GUIDrawText(const color32& color, const short2& coord, const char* text, TextAlign alignx, TextAlign aligny, float size, float lineSpacing)
    {
        if (m_gui_enabled)
        {
            m_gui_textBuilder.Initialize(text, m_gui_font, alignx, aligny, size, lineSpacing);
            
            auto rectCount = m_gui_textBuilder.GetVisibleGeometryCount();

            if (rectCount > 0)
            {
                auto idxv = m_gui_vertexCount;
                auto idxi = m_gui_indexCount;
                m_gui_vertexCount += rectCount * 4;
                m_gui_indexCount += rectCount * 6;

                if (m_gui_vertexCount <= GUI_MAX_VERTICES && m_gui_indexCount <= GUI_MAX_INDICES)
                {
                    const float2 texelSize = m_gui_font->GetRHI()->GetTexelSize().xy;

                    for (auto rect = m_gui_textBuilder.GetNextGeometry(); rect; rect = m_gui_textBuilder.GetNextGeometry())
                    {
                        if (!rect->isWhiteSpace)
                        {
                            const short4 sminmax = short4(rect->rect.x, rect->rect.y, rect->rect.x + rect->rect.z, rect->rect.y + rect->rect.w);
                            const float4 tminmax = float4(rect->texrect.x, rect->texrect.y, rect->texrect.x + rect->texrect.z, rect->texrect.y + rect->texrect.w);
                            m_gui_indexView[idxi++] = idxv + 0;
                            m_gui_indexView[idxi++] = idxv + 1;
                            m_gui_indexView[idxi++] = idxv + 2;
                            m_gui_indexView[idxi++] = idxv + 2;
                            m_gui_indexView[idxi++] = idxv + 3;
                            m_gui_indexView[idxi++] = idxv + 0;
                            m_gui_vertexView[idxv++] = { color, coord + sminmax.xy, Math::PackHalf(tminmax.xy * texelSize), GUI_TEX_INDEX_DEFAULT_FONT, 1u };
                            m_gui_vertexView[idxv++] = { color, coord + sminmax.xw, Math::PackHalf(tminmax.xw * texelSize), GUI_TEX_INDEX_DEFAULT_FONT, 1u };
                            m_gui_vertexView[idxv++] = { color, coord + sminmax.zw, Math::PackHalf(tminmax.zw * texelSize), GUI_TEX_INDEX_DEFAULT_FONT, 1u };
                            m_gui_vertexView[idxv++] = { color, coord + sminmax.zy, Math::PackHalf(tminmax.zy * texelSize), GUI_TEX_INDEX_DEFAULT_FONT, 1u };
                        }
                    }
                }
            }
        }
    }

    void EngineGUIRenderer::GUIDrawText(const color32& color, const short2& coord, const char* text, float size, float lineSpacing)
    {
        GUIDrawText(color, coord, text, TextAlign::Start, TextAlign::Start, size, lineSpacing);
    }


    void EngineGUIRenderer::GizmosDrawBounds(const BoundingBox& aabb)
    {
        if (m_gizmos_enabledCPU && Math::IntersectPlanesAABB(m_gizmos_frustrumPlanes.array_ptr(), 6, aabb))
        {
            auto idx = m_gizmos_vertexCount;
            m_gizmos_vertexCount += 24u;

            if (m_gizmos_vertexCount <= m_gizmos_maxVertices)
            {
                auto min = aabb.min;
                auto max = aabb.max;
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(min.x, min.y, min.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(min.x, max.y, min.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(max.x, min.y, min.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(max.x, max.y, min.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(min.x, min.y, max.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(min.x, max.y, max.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(max.x, min.y, max.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(max.x, max.y, max.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(min.x, min.y, min.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(max.x, min.y, min.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(min.x, max.y, min.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(max.x, max.y, min.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(min.x, min.y, max.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(max.x, min.y, max.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(min.x, max.y, max.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(max.x, max.y, max.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(min.x, min.y, min.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(min.x, min.y, max.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(max.x, min.y, min.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(max.x, min.y, max.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(min.x, max.y, min.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(min.x, max.y, max.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(max.x, max.y, min.z, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(max.x, max.y, max.z, 1.0f), m_gizmos_color };
            }
        }
    }

    void EngineGUIRenderer::GizmosDrawBox(const float3& origin, const float3& size)
    {
        GizmosDrawBounds(BoundingBox::MinMax(origin, origin + size));
    }

    void EngineGUIRenderer::GizmosDrawLine(const float3& start, const float3& end)
    {
        if (m_gizmos_enabledCPU)
        {
            auto idx = m_gizmos_vertexCount;
            m_gizmos_vertexCount += 2u;

            if (m_gizmos_vertexCount <= m_gizmos_maxVertices)
            {
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(start, 1.0f), m_gizmos_color };
                m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(end, 1.0f),   m_gizmos_color };
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
                    m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(nearCorners[i], 1.0f),           m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(nearCorners[(i + 1) % 4], 1.0f), m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(farCorners[i], 1.0f),            m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(farCorners[(i + 1) % 4], 1.0f),  m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(nearCorners[i], 1.0f),           m_gizmos_color };
                    m_gizmos_vertexView[idx++] = { m_gizmos_matrix * float4(farCorners[i], 1.0f),            m_gizmos_color };
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
        m_gizmos_frustrumPlanes = Math::ExtractFrustrumPlanes(vp, true);
        m_gizmos_matrix = matrix;
    }
}
