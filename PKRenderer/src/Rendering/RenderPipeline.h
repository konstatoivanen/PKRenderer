#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "Core/ApplicationConfig.h"
#include "ECS/Tokens/ViewProjectionToken.h"
#include "ECS/Tokens/TimeToken.h"
#include "ECS/Tokens/RenderingTokens.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "Rendering/Structs/GBuffers.h"
#include "Rendering/Passes/PassHierarchicalDepth.h"
#include "Rendering/Passes/PassEnvBackground.h"
#include "Rendering/Passes/PassPostEffects.h"
#include "Rendering/Passes/PassLights.h"
#include "Rendering/Passes/PassSceneGI.h"
#include "Rendering/Passes/PassVolumeFog.h"
#include "Rendering/Passes/PassFilmGrain.h"
#include "Rendering/Passes/PassDepthOfField.h"
#include "Rendering/Passes/PassTemporalAntiAliasing.h"
#include "Rendering/Passes/PassAutoExposure.h"
#include "Rendering/Passes/PassBloom.h"
#include "Rendering/Passes/Batcher.h"

namespace PK::Rendering
{
    class RenderPipeline : public Core::Services::IService,
                           public Core::Services::IStep<PK::ECS::Tokens::ViewProjectionUpdateToken>,
                           public Core::Services::IStep<PK::ECS::Tokens::TimeToken>,
                           public Core::Services::IConditionalStep<RHI::Window>,
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
            void Step(RHI::Window* window, int condition) final;
            void Step(Core::Services::AssetImportToken<Core::ApplicationConfig>* token) final;

        private:
            void DispatchRenderEvent(RHI::Objects::CommandBuffer* cmd, ECS::Tokens::RenderEvent renderEvent, const char* name, uint32_t* outPassGroup);

            Passes::PassLights m_passLights;
            Passes::PassSceneGI m_passSceneGI;
            Passes::PassVolumeFog m_passVolumeFog;

            Passes::PassHierarchicalDepth m_passHierarchicalDepth;
            Passes::PassEnvBackground m_passEnvBackground;
            Passes::PassFilmGrain m_passFilmGrain;
            Passes::PassDepthOfField m_depthOfField;
            Passes::PassTemporalAntialiasing m_temporalAntialiasing;
            Passes::PassBloom m_bloom;
            Passes::PassAutoExposure m_autoExposure;
            Passes::PassPostEffectsComposite m_passPostEffectsComposite;
            Passes::Batcher m_batcher;
            
            Core::Services::Sequencer* m_sequencer;

            RHI::Objects::AccelerationStructureRef m_sceneStructure;
            Rendering::Objects::ConstantBufferRef m_constantsPerFrame;
            GBuffersFull m_gbuffers;
            
            ECS::Tokens::VisibilityList m_visibilityList;
            Math::float4x4 m_worldToClip;
            uint64_t m_resizeFrameIndex;
            uint32_t m_forwardPassGroup;
            float m_znear;
            float m_zfar;
    };
}