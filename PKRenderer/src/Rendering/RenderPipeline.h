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
#include "Rendering/Passes/PassSceneGI.h"
#include "Rendering/Passes/PassVolumeFog.h"
#include "Rendering/Batcher.h"

namespace PK::Rendering
{
    class RenderPipeline : public Core::Services::IService,
                           public Core::Services::IStep<PK::ECS::Tokens::ViewProjectionUpdateToken>,
                           public Core::Services::IStep<PK::ECS::Tokens::TimeToken>,
                           public Core::Services::IConditionalStep<Core::Window>,
                           public Core::Services::IStep<Core::Services::AssetImportToken<Core::ApplicationConfig>>
    {
        public:
            RenderPipeline(Core::Services::AssetDatabase* assetDatabase, 
                           ECS::EntityDatabase* entityDb, 
                           Core::Services::Sequencer* sequencer, 
                           const Core::ApplicationConfig* config);

            ~RenderPipeline();

            void Step(PK::ECS::Tokens::ViewProjectionUpdateToken* token) override final;
            void Step(PK::ECS::Tokens::TimeToken* token) override final;
            void Step(Core::Window* window, int condition) override final;
            void Step(Core::Services::AssetImportToken<Core::ApplicationConfig>* token) override final;

        private:
            Passes::PassPostEffects m_passPostEffects;
            Passes::PassGeometry m_passGeometry;
            Passes::PassLights m_passLights;
            Passes::PassSceneGI m_passSceneGI;
            Passes::PassVolumeFog m_passVolumeFog;
            Batcher m_batcher;
            Core::Services::Sequencer* m_sequencer;

            Utilities::Ref<Objects::AccelerationStructure> m_sceneStructure;
            Utilities::Ref<Objects::ConstantBuffer> m_constantsPerFrame;
            Utilities::Ref<Objects::RenderTexture> m_renderTarget;
            Utilities::Ref<Objects::Texture> m_depthPrevious;
            Objects::Shader* m_OEMBackgroundShader;
            ECS::Tokens::VisibilityList m_visibilityList;

            Math::float4x4 m_viewProjectionMatrix;
            float m_znear;
            float m_zfar;
    };
}