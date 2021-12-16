#pragma once
#include "Core/IService.h"
#include "GraphicsAPI.h"
#include "ECS/Sequencer.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Structs/StructsCommon.h"
#include "ECS/Contextual/Tokens/ViewProjectionToken.h"
#include "ECS/Contextual/Tokens/TimeToken.h"
#include "Core/ApplicationConfig.h"
#include "Core/Window.h"

namespace PK::Rendering
{
    using namespace PK::Rendering::Objects;

    class RenderPipeline : public PK::Core::IService,
                           public PK::ECS::IStep<PK::ECS::Tokens::ViewProjectionUpdateToken>,
                           public PK::ECS::IStep<PK::ECS::Tokens::TimeToken>,
                           public PK::ECS::IConditionalStep<PK::Core::Window>
    {
        public:
            RenderPipeline(AssetDatabase* assetDatabase, const ApplicationConfig* config);
            ~RenderPipeline();

            void Step(PK::ECS::Tokens::ViewProjectionUpdateToken* token) override;
            void Step(PK::ECS::Tokens::TimeToken* token) override;
            void Step(Window* window, int condition) override;

        private:
            Ref<ConstantBuffer> m_constantsPerFrame;
            Ref<RenderTexture> m_HDRRenderTarget;
            Ref<RenderTexture> m_testTarget;
            Shader* m_OEMBackgroundShader;
            Texture* m_OEMTexture;
            float m_OEMExposure;

            Ref<Buffer> m_modelMatrices;
            Mesh* m_mesh;
            Texture* m_testTexture;
            Shader* m_shader = nullptr;
            Shader* m_blitTestShader = nullptr;

    };
}