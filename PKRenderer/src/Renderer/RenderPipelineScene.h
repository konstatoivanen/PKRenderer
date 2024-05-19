#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/IService.h"
#include "Renderer/Passes/PassHierarchicalDepth.h"
#include "Renderer/Passes/PassEnvBackground.h"
#include "Renderer/Passes/PassPostEffects.h"
#include "Renderer/Passes/PassLights.h"
#include "Renderer/Passes/PassSceneGI.h"
#include "Renderer/Passes/PassVolumeFog.h"
#include "Renderer/Passes/PassFilmGrain.h"
#include "Renderer/Passes/PassDepthOfField.h"
#include "Renderer/Passes/PassTemporalAntialiasing.h"
#include "Renderer/Passes/PassAutoExposure.h"
#include "Renderer/Passes/PassBloom.h"
#include "Renderer/IRenderPipeline.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)

namespace PK::Renderer
{
    class RenderPipelineScene : public IRenderPipeline,
        public Core::ControlFlow::IStep<Core::Assets::AssetImportEvent<Core::ApplicationConfig>*>
    {
        public:
            RenderPipelineScene(ECS::EntityDatabase* entityDb, Core::Assets::AssetDatabase* assetDatabase, Core::ApplicationConfig* config);
            ~RenderPipelineScene();

            const Graphics::RHI::BufferLayout& GetViewConstantsLayout() const final { return m_constantsLayout; }

            GBuffersFullDescriptor GetViewGBufferDescriptors() const final;

            void SetViewConstants(RenderView* view) final;

            void RenderViews(struct RenderPipelineContext* context) final;

            void Step(Core::Assets::AssetImportEvent<Core::ApplicationConfig>* token) final;

        private:
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
            Graphics::AccelerationStructureRef m_sceneStructure;
            Graphics::RHI::BufferLayout m_constantsLayout;

            // @TODO fix hack
            float m_backgroundExposure = 1.0f;
            
    };
}