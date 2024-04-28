#pragma once
#include "Utilities/ForwardDeclareUtility.h"
#include "Core/Services/IService.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "Rendering/Geometry/IGizmos.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::ControlFlow, class Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering, struct RenderPipelineEvent)

namespace PK::Engines
{
    class EngineGizmos : public Core::Services::IService, 
        public Core::ControlFlow::IStep<Rendering::RenderPipelineEvent*>,
        public Core::ControlFlow::IStep<Core::Assets::AssetImportEvent<Core::ApplicationConfig>*>,
        public PK::Rendering::Geometry::IGizmos
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

            virtual void Step(Rendering::RenderPipelineEvent* renderEvent) final;
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
            Rendering::RHI::Objects::Shader* m_gizmosShader = nullptr;
            Rendering::RHI::Objects::BufferRef m_vertexBuffer;
            Rendering::RHI::Objects::BufferRef m_indirectVertexBuffer;
            Rendering::RHI::Objects::BufferRef m_indirectArgsBuffer;
            PK::Utilities::BufferView<Vertex> m_vertexView;
            PK::Rendering::RHI::FixedFunctionShaderAttributes m_fixedFunctionAttribs;

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