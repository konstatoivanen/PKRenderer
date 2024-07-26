#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
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
        m_textures(MAX_TEXTURES),
        m_sequencer(sequencer)
    {
        m_guiShader = assetDatabase->Find<ShaderAsset>("VS_GUI");
        m_font = assetDatabase->Load<Font>("Content/Fonts/FSEX302.pkfont");
        m_vertexBuffer = RHI::CreateBuffer<GUIVertex>(MAX_VERTICES, BufferUsage::PersistentStorage, "GUI.VertexBuffer");
        m_indexBuffer = RHI::CreateBuffer<uint16_t>(MAX_INDICES, BufferUsage::DefaultIndex | BufferUsage::PersistentStage, "GUI.IndexBuffer");

        auto hash = HashCache::Get();

        SamplerDescriptor samplerDesc{};
        samplerDesc.anisotropy = 1.0f;
        samplerDesc.filterMin = FilterMode::Bilinear;
        samplerDesc.filterMag = FilterMode::Bilinear;
        samplerDesc.wrap[0] = WrapMode::Repeat;
        samplerDesc.wrap[1] = WrapMode::Repeat;
        samplerDesc.wrap[2] = WrapMode::Repeat;
        samplerDesc.normalized = true;
        RHI::SetSampler(hash->pk_Sampler_GUI, samplerDesc);
        RHI::SetBuffer(hash->pk_GUI_Vertices, m_vertexBuffer.get());

        CVariableRegister::Create<bool*>("Engine.GUI.Enabled", &m_enabled, "0 = 0ff, 1 = On", 1u);
        CVariableRegister::Create<CVariableFuncSimple>("Engine.GUI.Toggle", [this]() { m_enabled^= true; });
    }
    void EngineGUIRenderer::Step(RenderPipelineEvent* renderEvent)
    {
        if (!m_enabled)
        {
            return;
        }
        
        auto view = renderEvent->context->views[0];

        switch (renderEvent->type)
        {
            case RenderPipelineEvent::CollectDraws:
            {
                m_vertexCount = 0u;
                m_indexCount = 0u;
                m_renderAreaRect = view->renderAreaRect;
                m_vertexView = renderEvent->cmd.BeginBufferWrite<GUIVertex>(m_vertexBuffer.get());
                m_indexView = renderEvent->cmd.BeginBufferWrite<uint16_t>(m_indexBuffer.get());
                m_textures.Clear();
                m_textures.Set(RHI::GetBuiltInResources()->WhiteTexture2D.get());
                m_textures.Set(RHI::GetBuiltInResources()->ErrorTexture2D.get());
                m_textures.Set(m_font->GetRHI());
                m_sequencer->Next<IGUIRenderer*>(this, this);
                renderEvent->cmd->EndBufferWrite(m_vertexBuffer.get());
                renderEvent->cmd->EndBufferWrite(m_indexBuffer.get());
            }
            return;
            case RenderPipelineEvent::AfterPostEffects:
            {
                if (m_vertexCount >= 2)
                {
                    auto gbuffers = view->GetGBuffersFullView();
                    auto rect = gbuffers.current.color->GetRect();
                    auto hash = HashCache::Get();
                    RHI::SetTextureArray(hash->pk_GUI_Textures, m_textures);
                    renderEvent->cmd->SetIndexBuffer(m_indexBuffer.get(), ElementType::Ushort);
                    renderEvent->cmd.SetShader(m_guiShader);
                    renderEvent->cmd.SetRenderTarget(gbuffers.current.color);
                    renderEvent->cmd.SetViewPort(rect);
                    renderEvent->cmd.SetScissor(rect);
                    renderEvent->cmd->DrawIndexed(glm::min(MAX_INDICES, m_indexCount), 1u, 0u, 0u, 0u);
                }
            }
            return;

            default: return;
        }
    }

    uint16_t EngineGUIRenderer::AddTexture(RHITexture* texture)
    {
        if (texture == nullptr || texture->IsTracked())
        {
            PK_LOG_WARNING("Trying to add null or non readonly texture to be drawn in gui!");
            // Return error tex handle
            return TEX_INDEX_ERROR;
        }

        return (uint16_t)m_textures.Set(texture);
    }

    void EngineGUIRenderer::DrawTriangle(const GUIVertex& a, const GUIVertex& b, const GUIVertex& c)
    {
        auto idxv = m_vertexCount;
        auto idxi = m_indexCount;
        m_vertexCount += 3u;
        m_indexCount += 3u;

        if (m_vertexCount <= MAX_VERTICES && m_indexCount <= MAX_INDICES)
        {
            m_indexView[idxi++] = idxv + 0;
            m_indexView[idxi++] = idxv + 1;
            m_indexView[idxi++] = idxv + 2;
            m_vertexView[idxv++] = a;
            m_vertexView[idxv++] = b;
            m_vertexView[idxv++] = c;
        }
    }

    void EngineGUIRenderer::DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, RHITexture* texture)
    {
        DrawRect(color, rect, textureRect, AddTexture(texture));
    }

    void EngineGUIRenderer::DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, uint16_t textureIndex)
    {
        auto idxv = m_vertexCount;
        auto idxi = m_indexCount;
        m_vertexCount += 4u;
        m_indexCount += 6u;

        if (m_vertexCount <= MAX_VERTICES && m_indexCount <= MAX_INDICES)
        {
            const short4 sminmax = short4(rect.x, rect.y, rect.x + rect.z, rect.y + rect.w);
            const float4 tminmax = float4(textureRect.x, textureRect.y, textureRect.x + textureRect.z, textureRect.y + textureRect.w);
            const float2 texelSize = m_textures.Get(textureIndex)->GetTexelSize().xy;
            m_indexView[idxi++] = idxv + 0;
            m_indexView[idxi++] = idxv + 1;
            m_indexView[idxi++] = idxv + 2;
            m_indexView[idxi++] = idxv + 2;
            m_indexView[idxi++] = idxv + 3;
            m_indexView[idxi++] = idxv + 0;
            m_vertexView[idxv++] = { color, sminmax.xy, Math::PackHalf(tminmax.xy * texelSize), textureIndex, 0u };
            m_vertexView[idxv++] = { color, sminmax.xw, Math::PackHalf(tminmax.xw * texelSize), textureIndex, 0u };
            m_vertexView[idxv++] = { color, sminmax.zw, Math::PackHalf(tminmax.zw * texelSize), textureIndex, 0u };
            m_vertexView[idxv++] = { color, sminmax.zy, Math::PackHalf(tminmax.zy * texelSize), textureIndex, 0u };
        }
    }

    void EngineGUIRenderer::DrawRect(const color32& color, const short4& rect)
    {
        DrawRect(color, rect, PK_SHORT4_ZERO, TEX_INDEX_WHITE);
    }

    void EngineGUIRenderer::DrawText(const color32& color, const short2& coord, const char* text, TextAlign alignx, TextAlign aligny, float size, float lineSpacing)
    {
        m_textBuilder.Initialize(text, m_font, alignx, aligny, size, lineSpacing);
        
        auto rectCount = m_textBuilder.GetVisibleGeometryCount();

        if (rectCount == 0)
        {
            return;
        }

        auto idxv = m_vertexCount;
        auto idxi = m_indexCount;
        m_vertexCount += rectCount * 4;
        m_indexCount += rectCount * 6;

        if (m_vertexCount <= MAX_VERTICES && m_indexCount <= MAX_INDICES)
        {
            const float2 texelSize = m_font->GetRHI()->GetTexelSize().xy;

            for (auto rect = m_textBuilder.GetNextGeometry(); rect; rect = m_textBuilder.GetNextGeometry())
            {
                if (!rect->isWhiteSpace)
                {
                    const short4 sminmax = short4(rect->rect.x, rect->rect.y, rect->rect.x + rect->rect.z, rect->rect.y + rect->rect.w);
                    const float4 tminmax = float4(rect->texrect.x, rect->texrect.y, rect->texrect.x + rect->texrect.z, rect->texrect.y + rect->texrect.w);
                    m_indexView[idxi++] = idxv + 0;
                    m_indexView[idxi++] = idxv + 1;
                    m_indexView[idxi++] = idxv + 2;
                    m_indexView[idxi++] = idxv + 2;
                    m_indexView[idxi++] = idxv + 3;
                    m_indexView[idxi++] = idxv + 0;
                    m_vertexView[idxv++] = { color, coord + sminmax.xy, Math::PackHalf(tminmax.xy * texelSize), TEX_INDEX_DEFAULT_FONT, 1u };
                    m_vertexView[idxv++] = { color, coord + sminmax.xw, Math::PackHalf(tminmax.xw * texelSize), TEX_INDEX_DEFAULT_FONT, 1u };
                    m_vertexView[idxv++] = { color, coord + sminmax.zw, Math::PackHalf(tminmax.zw * texelSize), TEX_INDEX_DEFAULT_FONT, 1u };
                    m_vertexView[idxv++] = { color, coord + sminmax.zy, Math::PackHalf(tminmax.zy * texelSize), TEX_INDEX_DEFAULT_FONT, 1u };
                }
            }
        }
    }

    void EngineGUIRenderer::DrawText(const color32& color, const short2& coord, const char* text, float size, float lineSpacing)
    {
        DrawText(color, coord, text, TextAlign::Start, TextAlign::Start, size, lineSpacing);
    }
}