#pragma once
#include "Utilities/ForwardDeclareUtility.h"
#include "Core/Services/IService.h"
#include "Core/ControlFlow/IStep.h"
#include "Rendering/IRenderPipeline.h"
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
#include "Rendering/RHI/Objects/AccelerationStructure.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)

namespace PK::Rendering
{
    class RenderPipelineScene : public IRenderPipeline,
        public Core::ControlFlow::IStep<Core::Assets::AssetImportEvent<Core::ApplicationConfig>*>
    {
        public:
            RenderPipelineScene(ECS::EntityDatabase* entityDb, Core::Assets::AssetDatabase* assetDatabase, Core::ApplicationConfig* config);
            ~RenderPipelineScene();

            virtual const RHI::BufferLayout& GetViewConstantsLayout() const { return m_constantsLayout; }

            virtual Rendering::Objects::GBuffersFull::Descriptor GetViewGBufferDescriptors() const;

            virtual void SetViewConstants(Rendering::Objects::RenderView* view) final;

            virtual void RenderViews(struct RenderPipelineContext* context) final;

            virtual void Step(Core::Assets::AssetImportEvent<Core::ApplicationConfig>* token) final;

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
            RHI::Objects::AccelerationStructureRef m_sceneStructure;
            RHI::BufferLayout m_constantsLayout;

            // @TODO fix hack
            float m_backgroundExposure = 1.0f;
            
    };
}