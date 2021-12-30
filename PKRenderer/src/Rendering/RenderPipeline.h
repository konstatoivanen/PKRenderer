#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "Core/ApplicationConfig.h"
#include "Core/Window.h"
#include "ECS/Contextual/Tokens/ViewProjectionToken.h"
#include "ECS/Contextual/Tokens/TimeToken.h"
#include "ECS/Contextual/Tokens/CullingTokens.h"
#include "Rendering/Passes/PassPostEffects.h"
#include "Rendering/Passes/PassGeometry.h"
#include "Rendering/Passes/PassLights.h"
#include "Rendering/Batcher.h"

namespace PK::Rendering
{
    using namespace PK::Core;
    using namespace PK::Core::Services;
    using namespace PK::Rendering::Objects;

    class RenderPipeline : public IService,
                           public IStep<PK::ECS::Tokens::ViewProjectionUpdateToken>,
                           public IStep<PK::ECS::Tokens::TimeToken>,
                           public IConditionalStep<Window>,
                           public IStep<AssetImportToken<ApplicationConfig>>
    {
        public:
            RenderPipeline(AssetDatabase* assetDatabase, EntityDatabase* entityDb, Sequencer* sequencer, const ApplicationConfig* config);
            ~RenderPipeline();

            void Step(PK::ECS::Tokens::ViewProjectionUpdateToken* token) override final;
            void Step(PK::ECS::Tokens::TimeToken* token) override final;
            void Step(Window* window, int condition) override final;
            void Step(AssetImportToken<ApplicationConfig>* token) override final;

        private:
            Passes::PassPostEffects m_passPostEffects;
            Passes::PassGeometry m_passGeometry;
            Passes::PassLights m_passLights;
            Batcher m_batcher;

            Ref<ConstantBuffer> m_constantsPerFrame;
            Ref<RenderTexture> m_HDRRenderTarget;
            Shader* m_OEMBackgroundShader;
            ECS::Tokens::VisibilityList m_visibilityList;

            float4x4 m_viewProjectionMatrix;
            float m_znear;
            float m_zfar;
    };
}