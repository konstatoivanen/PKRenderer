#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/RHI/Layout.h"
#include "Core/Rendering/Font.h"
#include "Core/Rendering/RenderingFwd.h"
#include "App/Renderer/IGUIRenderer.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct Sequencer)

namespace PK::App
{
    struct RenderPipelineEvent;

    struct GizmosVertex
    {
        float3 pos;
        color32 color;
    };

    class EngineGUIRenderer :
        public IStep<RenderPipelineEvent*>,
        public IGUIRenderer,
        public IGizmosRenderer
    {
    public:
        EngineGUIRenderer(AssetDatabase* assetDatabase, Sequencer* sequencer);

        virtual void Step(RenderPipelineEvent* renderEvent) final;

        inline void SetGUIEnabled(bool value) { m_gui_enabled = value; }
        inline void SetGizmosEnabledGPU(bool value) { m_gizmos_enabledGPU = value; }
        inline void SetGizmosEnabledCPU(bool value) { m_gizmos_enabledCPU = value; }

        short4 GUIGetRenderAreaRect() const final { return m_gui_renderAreaRect; }
        uint16_t GUIAddTexture(RHITexture* texture) final;
        void GUIDrawTriangle(const GUIVertex& a, const GUIVertex& b, const GUIVertex& c) final;
        void GUIDrawRect(const color32& color, const short4& screenRect, const ushort4& textureRect, RHITexture* texture) final;
        void GUIDrawRect(const color32& color, const short4& screenRect, const ushort4& textureRect, uint16_t textureIndex) final;
        void GUIDrawRect(const color32& color, const short4& screenRect) final;
        void GUIDrawWireRect(const color32& color, const short4& rect, short inset) final;
        void GUIDrawLine(const color32& color0, const color32& color1, const short2& p0, const short2& p1, const float width) final;
        void GUIDrawText(const color32& color, const short2& coord, const char* text, TextAlign alignx, TextAlign aligny, float size = 1.0f, float lineSpacing = 1.0f) final;
        void GUIDrawText(const color32& color, const short2& coord, const char* text, float size = 1.0f, float lineSpacing = 1.0f) final;

        void GizmosDrawBounds(const BoundingBox& aabb) final;
        void GizmosDrawBox(const float3& origin, const float3& size) final;
        void GizmosDrawLine(const float3& start, const float3& end) final;
        void GizmosDrawRay(const float3& origin, const float3& vector) final;
        void GizmosDrawFrustrum(const float4x4& matrix) final;
        void GizmosSetColor(const color& color) final;
        void GizmosSetMatrix(const float4x4& matrix) final;

    private:
        constexpr static const uint32_t GUI_MAX_VERTICES = 16384u;
        constexpr static const uint32_t GUI_MAX_INDICES = GUI_MAX_VERTICES * 3;
        constexpr static const uint32_t GUI_MAX_TEXTURES = 64;
        constexpr static const uint16_t GUI_TEX_INDEX_WHITE = 0u;
        constexpr static const uint16_t GUI_TEX_INDEX_ERROR = 1u;
        constexpr static const uint16_t GUI_TEX_INDEX_DEFAULT_FONT = 2u;

        Sequencer* m_sequencer = nullptr;

        RHITextureBindSetRef m_gui_textures;
        ShaderAsset* m_gui_shader = nullptr;
        Font* m_gui_font = nullptr;
        RHIBufferRef m_gui_vertexBuffer;
        RHIBufferRef m_gui_indexBuffer;
        BufferView<GUIVertex> m_gui_vertexView;
        BufferView<uint16_t> m_gui_indexView;
        TextGeometryBuilder m_gui_textBuilder;

        short4 m_gui_renderAreaRect = PK_SHORT4_ZERO;
        uint32_t m_gui_vertexCount = 0;
        uint32_t m_gui_indexCount = 0;
        bool m_gui_enabled = false;

        ShaderAsset* m_gizmos_shader = nullptr;
        RHIBufferRef m_gizmos_vertexBuffer;
        RHIBufferRef m_gizmos_indirectVertexBuffer;
        RHIBufferRef m_gizmos_indirectArgsBuffer;
        BufferView<GizmosVertex> m_gizmos_vertexView;
        FixedFunctionShaderAttributes m_gizmos_fixedFunctionAttribs;
        VertexStreamElement m_gizmos_vertexStreamElement;

        FrustumPlanes m_gizmos_frustrumPlanes{};
        uint32_t m_gizmos_vertexCount = 0;
        uint32_t m_gizmos_maxVertices = 4096u;
        color32 m_gizmos_color = PK_COLOR32_WHITE;
        float4x4 m_gizmos_worldToClip = PK_FLOAT4X4_IDENTITY;
        float4x4 m_gizmos_matrix = PK_FLOAT4X4_IDENTITY;
        bool m_gizmos_enabledCPU = false;
        bool m_gizmos_enabledGPU = false;
    };
}
