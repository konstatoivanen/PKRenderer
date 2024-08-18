#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/RHI/Layout.h"
#include "Core/Rendering/BindSet.h"
#include "Core/Rendering/Font.h"
#include "Core/Rendering/RenderingFwd.h"
#include "App/Renderer/IGUIRenderer.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct Sequencer)

namespace PK::App
{
    struct RenderPipelineEvent;

    class EngineGUIRenderer :
        public IStep<RenderPipelineEvent*>,
        public IGUIRenderer
    {
    public:
        EngineGUIRenderer(AssetDatabase* assetDatabase, Sequencer* sequencer);

        virtual void Step(RenderPipelineEvent* renderEvent) final;

        inline void SetEnabled(bool value) { m_enabled = value; }

        virtual short4 GetRenderAreaRect() const final { return m_renderAreaRect; }
        virtual uint16_t AddTexture(RHITexture* texture) final;
        virtual void DrawTriangle(const GUIVertex& a, const GUIVertex& b, const GUIVertex& c) final;
        virtual void DrawRect(const color32& color, const short4& screenRect, const ushort4& textureRect, RHITexture* texture) final;
        virtual void DrawRect(const color32& color, const short4& screenRect, const ushort4& textureRect, uint16_t textureIndex) final;
        virtual void DrawRect(const color32& color, const short4& screenRect) final;
        virtual void DrawWireRect(const color32& color, const short4& rect, short inset) final;
        virtual void DrawText(const color32& color, const short2& coord, const char* text, TextAlign alignx, TextAlign aligny, float size = 1.0f, float lineSpacing = 1.0f) final;
        virtual void DrawText(const color32& color, const short2& coord, const char* text, float size = 1.0f, float lineSpacing = 1.0f) final;

    private:
        constexpr static const uint32_t MAX_VERTICES = 16384u;
        constexpr static const uint32_t MAX_INDICES = MAX_VERTICES * 3;
        constexpr static const uint32_t MAX_TEXTURES = 64;
        constexpr static const uint16_t TEX_INDEX_WHITE = 0u;
        constexpr static const uint16_t TEX_INDEX_ERROR = 1u;
        constexpr static const uint16_t TEX_INDEX_DEFAULT_FONT = 2u;

        BindSet<RHITexture> m_textures;
        Sequencer* m_sequencer = nullptr;
        ShaderAsset* m_guiShader = nullptr;
        Font* m_font = nullptr;
        RHIBufferRef m_vertexBuffer;
        RHIBufferRef m_indexBuffer;
        BufferView<GUIVertex> m_vertexView;
        BufferView<uint16_t> m_indexView;
        TextGeometryBuilder m_textBuilder;

        short4 m_renderAreaRect = PK_SHORT4_ZERO;
        uint32_t m_vertexCount = 0;
        uint32_t m_indexCount = 0;
        bool m_enabled = false;
    };
}