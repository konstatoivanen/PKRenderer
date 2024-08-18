#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/RHI/Layout.h"
#include "Core/Rendering/RenderingFwd.h"
#include "App/Renderer/IGizmos.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct Sequencer)

namespace PK::App
{
    struct RenderPipelineEvent;

    class EngineGizmos : 
        public IStep<RenderPipelineEvent*>,
        public IGizmos
    {
    public:
        struct Vertex
        {
            float3 pos;
            color32 color;
        };

        EngineGizmos(AssetDatabase* assetDatabase, Sequencer* sequencer);

        virtual void Step(RenderPipelineEvent* renderEvent) final;

        inline void SetEnabledGPU(bool value) { m_enabledGPU = value; }
        inline void SetEnabledCPU(bool value) { m_enabledCPU = value; }

        void DrawBounds(const BoundingBox& aabb) final;
        void DrawBox(const float3& origin, const float3& size) final;
        void DrawLine(const float3& start, const float3& end) final;
        void DrawRay(const float3& origin, const float3& vector) final;
        void DrawFrustrum(const float4x4& matrix) final;
        void SetColor(const color& color) final;
        void SetMatrix(const float4x4& matrix) final;

    private:
        Sequencer* m_sequencer = nullptr;
        ShaderAsset* m_gizmosShader = nullptr;
        RHIBufferRef m_vertexBuffer;
        RHIBufferRef m_indirectVertexBuffer;
        RHIBufferRef m_indirectArgsBuffer;
        BufferView<Vertex> m_vertexView;
        FixedFunctionShaderAttributes m_fixedFunctionAttribs;
        VertexStreamElement m_vertexStreamElement;

        FrustumPlanes m_frustrumPlanes{};
        uint32_t m_vertexCount = 0;
        uint32_t m_maxVertices = 4096u;
        color32 m_color = PK_COLOR32_WHITE;
        float4x4 m_worldToClip = PK_FLOAT4X4_IDENTITY;
        float4x4 m_matrix = PK_FLOAT4X4_IDENTITY;
        bool m_enabledCPU = false;
        bool m_enabledGPU = false;
    };
}