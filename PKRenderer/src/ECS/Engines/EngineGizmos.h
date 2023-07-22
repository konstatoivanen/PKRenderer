#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/Buffer.h"
#include "Rendering/Objects/Shader.h"
#include "ECS/Tokens/RenderingTokens.h"
#include "ECS/Tokens/GizmosToken.h"

namespace PK::ECS::Engines
{
    class EngineGizmos : public Core::Services::IService, 
                         public Core::Services::IConditionalStep<Tokens::TokenRenderEvent>,
                         public Core::Services::IStep<Core::Services::AssetImportToken<Core::ApplicationConfig>>,
                         public Core::Services::IStep<Core::TokenConsoleCommand>,
                         public Tokens::IGizmosRenderer
    {
        public:
            struct Vertex
            {
                Math::float3 pos;
                Math::color32 color;
            };

            EngineGizmos(Core::Services::AssetDatabase* assetDatabase,
                         Core::Services::Sequencer* sequencer,
                         Core::ApplicationConfig* config);

            void Step(Tokens::TokenRenderEvent* token, int condition) final;
            void Step(Core::Services::AssetImportToken<Core::ApplicationConfig>* token) final;
            void Step(Core::TokenConsoleCommand* token) final;

            void DrawBounds(const Math::BoundingBox& aabb) final;
            void DrawBox(const Math::float3& origin, const Math::float3& size) final;
            void DrawLine(const Math::float3& start, const Math::float3& end) final;
            void DrawRay(const Math::float3& origin, const Math::float3& vector) final;
            void DrawFrustrum(const Math::float4x4& matrix) final;
            void SetColor(const Math::color& color) final;
            void SetMatrix(const Math::float4x4& matrix) final;

        private:
            Core::Services::Sequencer* m_sequencer = nullptr;
            Rendering::Objects::Shader* m_gizmosShader = nullptr;
            Utilities::Ref<Rendering::Objects::Buffer> m_vertexBuffer;
            PK::Utilities::BufferView<Vertex> m_vertexView;
            PK::Rendering::Structs::FixedFunctionShaderAttributes m_fixedFunctionAttribs;

            Math::FrustumPlanes m_frustrumPlanes{};
            uint32_t m_vertexCount = 0;
            uint32_t m_maxVertices = 4096u;
            Math::color32 m_color = Math::PK_COLOR32_WHITE;
            Math::float4x4 m_viewprojection = Math::PK_FLOAT4X4_IDENTITY;
            Math::float4x4 m_matrix = Math::PK_FLOAT4X4_IDENTITY;
            bool m_enabled = true;
    };
}