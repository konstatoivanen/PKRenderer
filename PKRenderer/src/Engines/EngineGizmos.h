#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/IService.h"
#include "Core/ControlFlow/IStep.h"
#include "Graphics/RHI/Layout.h"
#include "Graphics/GraphicsFwd.h"
#include "Renderer/IGizmos.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::ControlFlow, class Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Renderer, struct RenderPipelineEvent)

namespace PK::Engines
{
    class EngineGizmos : public Core::IService,
        public Core::ControlFlow::IStep<Renderer::RenderPipelineEvent*>,
        public Core::ControlFlow::IStep<Core::Assets::AssetImportEvent<Core::ApplicationConfig>*>,
        public PK::Renderer::IGizmos
    {
    public:
        struct Vertex
        {
            Math::float3 pos;
            Math::color32 color;
        };

        EngineGizmos(Core::Assets::AssetDatabase* assetDatabase,
            Core::ControlFlow::Sequencer* sequencer,
            Core::ApplicationConfig* config);

        virtual void Step(Renderer::RenderPipelineEvent* renderEvent) final;
        virtual void Step(Core::Assets::AssetImportEvent<Core::ApplicationConfig>* token) final;

        inline void SetEnabledGPU(bool value) { m_enabledGPU = value; }
        inline void SetEnabledCPU(bool value) { m_enabledCPU = value; }

        void DrawBounds(const Math::BoundingBox& aabb) final;
        void DrawBox(const Math::float3& origin, const Math::float3& size) final;
        void DrawLine(const Math::float3& start, const Math::float3& end) final;
        void DrawRay(const Math::float3& origin, const Math::float3& vector) final;
        void DrawFrustrum(const Math::float4x4& matrix) final;
        void SetColor(const Math::color& color) final;
        void SetMatrix(const Math::float4x4& matrix) final;

    private:
        Core::ControlFlow::Sequencer* m_sequencer = nullptr;
        Graphics::Shader* m_gizmosShader = nullptr;
        Graphics::BufferRef m_vertexBuffer;
        Graphics::BufferRef m_indirectVertexBuffer;
        Graphics::BufferRef m_indirectArgsBuffer;
        PK::Utilities::BufferView<Vertex> m_vertexView;
        PK::Graphics::RHI::FixedFunctionShaderAttributes m_fixedFunctionAttribs;
        PK::Graphics::RHI::VertexStreamElement m_vertexStreamElement;

        Math::FrustumPlanes m_frustrumPlanes{};
        uint32_t m_vertexCount = 0;
        uint32_t m_maxVertices = 4096u;
        Math::color32 m_color = Math::PK_COLOR32_WHITE;
        Math::float4x4 m_worldToClip = Math::PK_FLOAT4X4_IDENTITY;
        Math::float4x4 m_matrix = Math::PK_FLOAT4X4_IDENTITY;
        bool m_enabledCPU = false;
        bool m_enabledGPU = false;
    };
}