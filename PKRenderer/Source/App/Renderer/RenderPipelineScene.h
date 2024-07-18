#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "App/Renderer/Passes/PassHierarchicalDepth.h"
#include "App/Renderer/Passes/PassEnvBackground.h"
#include "App/Renderer/Passes/PassPostEffects.h"
#include "App/Renderer/Passes/PassLights.h"
#include "App/Renderer/Passes/PassSceneGI.h"
#include "App/Renderer/Passes/PassVolumeFog.h"
#include "App/Renderer/Passes/PassFilmGrain.h"
#include "App/Renderer/Passes/PassDepthOfField.h"
#include "App/Renderer/Passes/PassTemporalAntialiasing.h"
#include "App/Renderer/Passes/PassAutoExposure.h"
#include "App/Renderer/Passes/PassBloom.h"
#include "App/Renderer/IRenderPipeline.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)

namespace PK::App
{
    class RenderPipelineScene : public IRenderPipeline
    {
        public:
            RenderPipelineScene(AssetDatabase* assetDatabase, const uint2& initialResolution);
            ~RenderPipelineScene();

            const BufferLayout& GetViewConstantsLayout() const final { return m_constantsLayout; }

            GBuffersFullDescriptor GetViewGBufferDescriptors() const final;

            void SetViewConstants(RenderView* view) final;

            void RenderViews(struct RenderPipelineContext* context) final;

        private:
            PassLights m_passLights;
            PassSceneGI m_passSceneGI;
            PassVolumeFog m_passVolumeFog;
            PassHierarchicalDepth m_passHierarchicalDepth;
            PassEnvBackground m_passEnvBackground;
            PassFilmGrain m_passFilmGrain;
            PassDepthOfField m_depthOfField;
            PassTemporalAntialiasing m_temporalAntialiasing;
            PassBloom m_bloom;
            PassAutoExposure m_autoExposure;
            PassPostEffectsComposite m_passPostEffectsComposite;
            
            RHIAccelerationStructureRef m_sceneStructure;
            BufferLayout m_constantsLayout;
    };
}