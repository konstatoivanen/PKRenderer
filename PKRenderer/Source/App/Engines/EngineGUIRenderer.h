#pragma once
#include "Core/ControlFlow/IStep.h"
#include "Core/RHI/Layout.h"
#include "Core/Rendering/Font.h"
#include "Core/Rendering/RenderingFwd.h"
#include "Core/GUI/GUIDrawList.h"
#include "App/Renderer/IGUIRenderer.h"

namespace PK { class AssetDatabase; }
namespace PK { struct Sequencer; }

namespace PK::App
{
    struct RenderPipelineEvent;

    struct GizmosVertex
    {
        float3 pos;
        color32 color;
    };

    struct EngineGUIRenderer :
        public IStep<RenderPipelineEvent*>,
        public IGUIAllocator,
        public IGizmosRenderer
    {
        EngineGUIRenderer(AssetDatabase* assetDatabase, Sequencer* sequencer);

        virtual void Step(RenderPipelineEvent* renderEvent) final;

        inline void SetGUIEnabled(bool value) { m_gui_enabled = value; }
        inline void SetGizmosEnabledGPU(bool value) { m_gizmos_enabledGPU = value; }
        inline void SetGizmosEnabledCPU(bool value) { m_gizmos_enabledCPU = value; }

        void GUICollectDraws(const uint4& renderArea, CommandBufferExt& cmd);
        void GUIDispatchDraws(CommandBufferExt& cmd, RHITexture* target);
        bool GUIValidateDraw();
        
        short4 GUIGetRenderAreaRect() const final;
        Font* GUIGetDefaultFont() const final;
        uint16_t GUIGetTextureIndex(RHITexture* texture) final;
        uint3 GUIGetTextureSize(uint16_t textureIndex) const final;
        bool GUIAllocate(uint32_t layer, uint32_t vertexCount, uint32_t indexCount, GUIAllocation* allocation) final;

        void GizmosCollectDraws(const uint4& renderArea, const float4x4& worldToClip, CommandBufferExt& cmd);
        void GizmosDispatchDraws(CommandBufferExt& cmd, RHITexture* target);
        void GizmosDrawBounds(const AABB<float3>& aabb) final;
        void GizmosDrawBox(const float3& origin, const float3& size) final;
        void GizmosDrawLine(const float3& start, const float3& end) final;
        void GizmosDrawRay(const float3& origin, const float3& vector) final;
        void GizmosDrawFrustrum(const float4x4& matrix) final;
        void GizmosSetColor(const color& color) final;
        void GizmosSetMatrix(const float4x4& matrix) final;
        const float4x4& GizmosGetWorldToClipMatrix() const final;
        const short4& GizmosGetRenderAreaRect() const final;

    private:
        constexpr static const uint32_t GUI_MAX_VERTICES = 16384u;
        constexpr static const uint32_t GUI_MAX_INDICES = GUI_MAX_VERTICES * 3u;
        constexpr static const uint32_t GUI_MAX_TEXTURES = 64;

        Sequencer* m_sequencer = nullptr;
        AssetDatabase* m_assetDatabase = nullptr;

        CommandBufferExt* m_gui_commandBuffer = nullptr;
        RHITextureBindSetRef m_gui_textures;
        ShaderAsset* m_gui_shader = nullptr;
        Font* m_gui_font = nullptr;
        RHIBufferRef m_gui_vertexBuffer;
        RHIBufferRef m_gui_indexBuffer;
        BufferView<GUIVertex> m_gui_vertexView;
        BufferView<GUIIndex> m_gui_indexView;
        short4 m_gui_renderAreaRect = PK_SHORT4_ZERO;
        uint32_t m_gui_vertexCount = 0u;
        uint32_t m_gui_indexCount = 0u;
        bool m_gui_enabled = true;

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
        short4 m_gizmos_renderAreaRect = PK_SHORT4_ZERO;
        color32 m_gizmos_color = PK_COLOR32_WHITE;
        float4x4 m_gizmos_worldToClip = PK_FLOAT4X4_IDENTITY;
        float4x4 m_gizmos_matrix = PK_FLOAT4X4_IDENTITY;
        bool m_gizmos_enabledCPU = false;
        bool m_gizmos_enabledGPU = false;
    };
}
