#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "ECS/EGID.h"
#include "Graphics/GraphicsFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Renderer, struct IGizmos)

namespace PK::Engines
{
    // Dumping ground for all loose hooks that have not been implemented yet.
    class EngineDebug : public Core::IService,
        public Core::ControlFlow::IStepApplicationUpdateEngines,
        public Core::ControlFlow::IStep<Renderer::IGizmos*>,
        public Core::ControlFlow::IStep<Core::Assets::AssetImportEvent<Core::ApplicationConfig>*>
    {
    public:
        EngineDebug(Core::Assets::AssetDatabase* assetDatabase,
            ECS::EntityDatabase* entityDb,
            PK::Graphics::MeshStaticCollection* baseMesh,
            const Core::ApplicationConfig* config);

        virtual void OnApplicationUpdateEngines() final;
        virtual void Step(Renderer::IGizmos* gizmos) final;
        virtual void Step(Core::Assets::AssetImportEvent<Core::ApplicationConfig>* token) final;

    private:
        ECS::EGID m_cameraEgid{};
        ECS::EntityDatabase* m_entityDb;
        Core::Assets::AssetDatabase* m_assetDatabase;
    };
}