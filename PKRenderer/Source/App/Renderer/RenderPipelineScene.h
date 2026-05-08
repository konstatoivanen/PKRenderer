#pragma once
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

namespace PK { class AssetDatabase; }
namespace PK { struct EntityDatabase; }

namespace PK::App
{    
    class RenderPipelineScene : public IRenderPipeline<
        PassLights::ViewResources,
        PassSceneGI::ViewResources,
        PassVolumetricFog::ViewResources,
        PassHierarchicalDepth::ViewResources,
        PassSceneEnv::ViewResources,
        PassDepthOfField::ViewResources,
        PassTemporalAntialiasing::ViewResources,
        PassDistort::ViewResources,
        PassBloom::ViewResources,
        PassAutoExposure::ViewResources>
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
            ShaderPropertyLayout m_constantsLayout;
    };
}
