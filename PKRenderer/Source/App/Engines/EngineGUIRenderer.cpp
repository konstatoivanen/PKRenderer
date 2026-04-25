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
        m_gizmos_vertexStreamElement.size = sizeof(uint4);

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
            m_sequencer->Next<IGUIRenderer*>(this, this);
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
            cmd->SetIndexBuffer(m_gui_indexBuffer.get(), ElementType::Ushort);
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
                m_gui_indexBuffer = RHI::CreateBuffer<uint16_t>(GUI_MAX_INDICES, BufferUsage::DefaultIndex | BufferUsage::PersistentStage, "GUI.IndexBuffer");
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

    uint16_t EngineGUIRenderer::GUIAddTexture(RHITexture* texture)
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

    void EngineGUIRenderer::GUIDrawTriangle(const GUIVertex& a, const GUIVertex& b, const GUIVertex& c)
    {
        if (GUIValidateDraw())
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
        if (GUIValidateDraw())
        {
            auto idxv = m_gui_vertexCount;
            auto idxi = m_gui_indexCount;
            m_gui_vertexCount += 4u;
            m_gui_indexCount += 6u;

            if (m_gui_vertexCount <= GUI_MAX_VERTICES && m_gui_indexCount <= GUI_MAX_INDICES)
            {
                const short4 sminmax = short4(rect.x, rect.y, rect.x + rect.z, rect.y + rect.w);
                const float4 tminmax = float4(textureRect.x, textureRect.y, textureRect.x + textureRect.z, textureRect.y + textureRect.w);
                const float2 texelSize = (1.0f / float3(m_gui_textures->GetBoundTextureSize(textureIndex))).xy;
                m_gui_indexView[idxi++] = idxv + 0;
                m_gui_indexView[idxi++] = idxv + 1;
                m_gui_indexView[idxi++] = idxv + 2;
                m_gui_indexView[idxi++] = idxv + 2;
                m_gui_indexView[idxi++] = idxv + 3;
                m_gui_indexView[idxi++] = idxv + 0;
                m_gui_vertexView[idxv++] = { color, sminmax.xy, math::f32tof16(tminmax.xy * texelSize), textureIndex, 0u };
                m_gui_vertexView[idxv++] = { color, sminmax.xw, math::f32tof16(tminmax.xw * texelSize), textureIndex, 0u };
                m_gui_vertexView[idxv++] = { color, sminmax.zw, math::f32tof16(tminmax.zw * texelSize), textureIndex, 0u };
                m_gui_vertexView[idxv++] = { color, sminmax.zy, math::f32tof16(tminmax.zy * texelSize), textureIndex, 0u };
            }
        }
    }

    void EngineGUIRenderer::GUIDrawRect(const color32& color, const short4& rect)
    {
        GUIDrawRect(color, rect, PK_USHORT4_ZERO, GUI_TEX_INDEX_WHITE);
    }

    void EngineGUIRenderer::GUIDrawWireRect(const color32& color, const short4& rect, short inset)
    {
        if (GUIValidateDraw())
        {
            auto idxv = m_gui_vertexCount;
            auto idxi = m_gui_indexCount;
            m_gui_vertexCount += 8u;
            m_gui_indexCount += 24u;

            if (m_gui_vertexCount <= GUI_MAX_VERTICES && m_gui_indexCount <= GUI_MAX_INDICES)
            {
                const short4 outer = short4(rect.x, rect.y, rect.x + rect.z, rect.y + rect.w);
                const short4 inner = short4(outer.x + inset, outer.y + inset, outer.z - inset, outer.w - inset);

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

                m_gui_vertexView[idxv + 0] = { color, outer.xy, PK_USHORT2_ZERO, 0, 0u };
                m_gui_vertexView[idxv + 1] = { color, inner.xy, PK_USHORT2_ZERO, 0, 0u };

                m_gui_vertexView[idxv + 2] = { color, outer.xw, PK_USHORT2_ZERO, 0, 0u };
                m_gui_vertexView[idxv + 3] = { color, inner.xw, PK_USHORT2_ZERO, 0, 0u };
                
                m_gui_vertexView[idxv + 4] = { color, outer.zw, PK_USHORT2_ZERO, 0, 0u };
                m_gui_vertexView[idxv + 5] = { color, inner.zw, PK_USHORT2_ZERO, 0, 0u };
                
                m_gui_vertexView[idxv + 6] = { color, outer.zy, PK_USHORT2_ZERO, 0, 0u };
                m_gui_vertexView[idxv + 7] = { color, inner.zy, PK_USHORT2_ZERO, 0, 0u };
            }
        }
    }

    void EngineGUIRenderer::GUIDrawLine(const color32& color0, const color32& color1, const short2& p0, const short2& p1, const float width)
    {
        if (GUIValidateDraw())
        {
            auto idxv = m_gui_vertexCount;
            auto idxi = m_gui_indexCount;
            m_gui_vertexCount += 4u;
            m_gui_indexCount += 6u;

            if (m_gui_vertexCount <= GUI_MAX_VERTICES && m_gui_indexCount <= GUI_MAX_INDICES)
            {
                const auto p0f = float2(p0.x + 0.5f, p0.y + 0.5f);
                const auto p1f = float2(p1.x + 0.5f, p1.y + 0.5f);
                const auto direction = math::normalize(p1f - p0f);
                const auto tangent = float2(-direction.y, direction.x);
                const auto offset = math::normalize(tangent + direction) * 0.5f * width;
 
                m_gui_indexView[idxi++] = idxv + 0;
                m_gui_indexView[idxi++] = idxv + 1;
                m_gui_indexView[idxi++] = idxv + 2;
                m_gui_indexView[idxi++] = idxv + 2;
                m_gui_indexView[idxi++] = idxv + 3;
                m_gui_indexView[idxi++] = idxv + 0;
                m_gui_vertexView[idxv++] = { color0, math::round(p0f + float2(-offset.y, +offset.x)), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, 0u };
                m_gui_vertexView[idxv++] = { color1, math::round(p1f + float2(+offset.x, +offset.y)), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, 0u };
                m_gui_vertexView[idxv++] = { color1, math::round(p1f + float2(+offset.y, -offset.x)), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, 0u };
                m_gui_vertexView[idxv++] = { color0, math::round(p0f + float2(-offset.x, -offset.y)), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, 0u };
            }
        }
    }

    short4 EngineGUIRenderer::GUIDrawText(const color32& color, const short4& rect, const char* text, const FontStyle& style)
    {
        auto text_area_min = +PK_SHORT2_MAX;
        auto text_area_max = -PK_SHORT2_MAX;
        
        if (GUIValidateDraw())
        {
            auto max_rects = Font::CalculateMaxRectCount(text, m_gui_font);
            
            if (max_rects > 0u)
            {
                auto text_rects = PK_STACK_ALLOC(FontRect, max_rects);
                auto rect_count = Font::CalculateRects(text, m_gui_font, rect, rect, style, text_rects, max_rects);

                if (rect_count > 0u)
                {
                    auto idxv = m_gui_vertexCount;
                    auto idxi = m_gui_indexCount;
                    m_gui_vertexCount += rect_count * 4u;
                    m_gui_indexCount += rect_count * 6u;

                    if (m_gui_vertexCount <= GUI_MAX_VERTICES && m_gui_indexCount <= GUI_MAX_INDICES)
                    {
                        const float2 texelSize = m_gui_font->GetRHI()->GetTexelSize().xy;

                        for (auto i = 0u; i < rect_count; ++i)
                        {
                            auto& crect = text_rects[i];
                            const auto sminmax = short4(crect.rect.x, crect.rect.y, crect.rect.x + crect.rect.z, crect.rect.y + crect.rect.w);
                            const auto tminmax = float4(crect.texrect.x, crect.texrect.y, crect.texrect.x + crect.texrect.z, crect.texrect.y + crect.texrect.w);
                            m_gui_indexView[idxi++] = idxv + 0;
                            m_gui_indexView[idxi++] = idxv + 1;
                            m_gui_indexView[idxi++] = idxv + 2;
                            m_gui_indexView[idxi++] = idxv + 2;
                            m_gui_indexView[idxi++] = idxv + 3;
                            m_gui_indexView[idxi++] = idxv + 0;
                            m_gui_vertexView[idxv++] = { color, sminmax.xy, math::f32tof16(tminmax.xy * texelSize), GUI_TEX_INDEX_DEFAULT_FONT, 1u };
                            m_gui_vertexView[idxv++] = { color, sminmax.xw, math::f32tof16(tminmax.xw * texelSize), GUI_TEX_INDEX_DEFAULT_FONT, 1u };
                            m_gui_vertexView[idxv++] = { color, sminmax.zw, math::f32tof16(tminmax.zw * texelSize), GUI_TEX_INDEX_DEFAULT_FONT, 1u };
                            m_gui_vertexView[idxv++] = { color, sminmax.zy, math::f32tof16(tminmax.zy * texelSize), GUI_TEX_INDEX_DEFAULT_FONT, 1u };

                            text_area_min.x = math::min(text_area_min.x, sminmax.x, sminmax.z);
                            text_area_min.y = math::min(text_area_min.y, sminmax.y, sminmax.w);
                            text_area_max.x = math::max(text_area_max.x, sminmax.x, sminmax.z);
                            text_area_max.y = math::max(text_area_max.y, sminmax.y, sminmax.w);
                        }
                    }
                }
            }
        }

        return { text_area_min, text_area_max - text_area_min };
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
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(min.x, min.y, min.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(min.x, max.y, min.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(max.x, min.y, min.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(max.x, max.y, min.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(min.x, min.y, max.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(min.x, max.y, max.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(max.x, min.y, max.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(max.x, max.y, max.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(min.x, min.y, min.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(max.x, min.y, min.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(min.x, max.y, min.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(max.x, max.y, min.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(min.x, min.y, max.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(max.x, min.y, max.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(min.x, max.y, max.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(max.x, max.y, max.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(min.x, min.y, min.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(min.x, min.y, max.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(max.x, min.y, min.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(max.x, min.y, max.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(min.x, max.y, min.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(min.x, max.y, max.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(max.x, max.y, min.z, 1.0f)).xyz, m_gizmos_color };
                m_gizmos_vertexView[idx++] = { (m_gizmos_matrix * float4(max.x, max.y, max.z, 1.0f)).xyz, m_gizmos_color };
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
        m_gizmos_frustrumPlanes = Math::ExtractFrustrumPlanes(vp, true);
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
