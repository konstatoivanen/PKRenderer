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
#include "Rendering/Passes/PassFilmGrain.h"
#include "Rendering/Passes/PassDepthOfField.h"
#include "Rendering/Passes/PassTemporalAntiAliasing.h"
#include "Rendering/Services/Batcher.h"

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
                           Core::ApplicationConfig* config);

            ~RenderPipeline();

            void Step(PK::ECS::Tokens::ViewProjectionUpdateToken* token) final;
            void Step(PK::ECS::Tokens::TimeToken* token) final;
            void Step(Core::Window* window, int condition) final;
            void Step(Core::Services::AssetImportToken<Core::ApplicationConfig>* token) final;

        private:
            void ComputeHierarchicalDepth(Objects::CommandBuffer* cmd);

            Passes::PassGeometry m_passGeometry;
            Passes::PassLights m_passLights;
            Passes::PassSceneGI m_passSceneGI;
            Passes::PassVolumeFog m_passVolumeFog;

            Passes::PassFilmGrain m_passFilmGrain;
            Passes::PassDepthOfField m_depthOfField;
            Passes::PassTemporalAntialiasing m_temporalAntialiasing;
            Passes::PassBloom m_bloom;
            Passes::PassHistogram m_histogram;
            Passes::PassPostEffectsComposite m_passPostEffectsComposite;

            Batcher m_batcher;
            Core::Services::Sequencer* m_sequencer;

            Utilities::Ref<Objects::AccelerationStructure> m_sceneStructure;
            Utilities::Ref<Objects::ConstantBuffer> m_constantsPostProcess;
            Utilities::Ref<Objects::ConstantBuffer> m_constantsPerFrame;
            Utilities::Ref<Objects::RenderTexture> m_renderTarget;
            Utilities::Ref<Objects::Texture> m_previousColor;
            Utilities::Ref<Objects::Texture> m_previousNormals;
            Utilities::Ref<Objects::Texture> m_previousDepth;
            Utilities::Ref<Objects::Texture> m_hierarchicalDepth;
            Objects::Shader* m_OEMBackgroundShader;
            Objects::Shader* m_computeHierachicalDepth;

            ECS::Tokens::VisibilityList m_visibilityList;
            Math::float4x4 m_viewProjectionMatrix;
            uint64_t m_resizeFrameIndex;
            float m_znear;
            float m_zfar;
    };
}