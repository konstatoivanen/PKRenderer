#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "App/Renderer/Passes/PassHierarchicalDepth.h"
#include "App/Renderer/Passes/PassSceneEnv.h"
#include "App/Renderer/Passes/PassPostEffects.h"
#include "App/Renderer/Passes/PassLights.h"
#include "App/Renderer/Passes/PassSceneGI.h"
#include "App/Renderer/Passes/PassVolumetricFog.h"
#include "App/Renderer/Passes/PassFilmGrain.h"
#include "App/Renderer/Passes/PassDistort.h"
#include "App/Renderer/Passes/PassDepthOfField.h"
#include "App/Renderer/Passes/PassTemporalAntialiasing.h"
#include "App/Renderer/Passes/PassAutoExposure.h"
#include "App/Renderer/Passes/PassBloom.h"
#include "App/Renderer/RenderPipelineBase.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)

namespace PK::App
{    
    // New format for per pass resources
    struct SceneRenderViewResources : 
        public IRenderViewResources,
        public PassLights::ViewResources,
        public PassSceneGI::ViewResources,
        public PassVolumetricFog::ViewResources,
        public PassHierarchicalDepth::ViewResources,
        public PassSceneEnv::ViewResources,
        public PassDepthOfField::ViewResources,
        public PassTemporalAntialiasing::ViewResources,
        public PassDistort::ViewResources,
        public PassBloom::ViewResources,
        public PassAutoExposure::ViewResources
    {
    };

    class RenderPipelineScene : public IRenderPipeline<SceneRenderViewResources>
    {
        public:
            RenderPipelineScene(AssetDatabase* assetDatabase,
                EntityDatabase* entityDb,
                Sequencer* sequencer,
                IBatcher* batcher);

            ~RenderPipelineScene();

            void Render(struct RenderPipelineContext* context) final;

        private:
            PassLights m_passLights;
            PassSceneGI m_passSceneGI;
            PassVolumetricFog m_passVolumetricFog;
            PassHierarchicalDepth m_passHierarchicalDepth;
            PassSceneEnv m_passSceneEnv;
            PassFilmGrain m_passFilmGrain;
            PassDepthOfField m_depthOfField;
            PassTemporalAntialiasing m_temporalAntialiasing;
            PassDistort m_distort;
            PassBloom m_bloom;
            PassAutoExposure m_autoExposure;
            PassPostEffectsComposite m_passPostEffectsComposite;
            
            RHIAccelerationStructureRef m_sceneStructure;
            ShaderStructLayout m_constantsLayout;
    };
}
