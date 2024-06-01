#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ECS/EGID.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "Core/Rendering/RenderingFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)

namespace PK::App
{
    struct RendererConfig;
    struct IGizmos;

    // Dumping ground for all loose hooks that have not been implemented yet.
    class EngineDebug : 
        public IStepApplicationUpdateEngines,
        public IStep<IGizmos*>,
        public IStep<AssetImportEvent<RendererConfig>*>
    {
    public:
        EngineDebug(AssetDatabase* assetDatabase,
            EntityDatabase* entityDb,
            MeshStaticCollection* baseMesh,
            const RendererConfig* config);

        virtual void OnApplicationUpdateEngines() final;
        virtual void Step(IGizmos* gizmos) final;
        virtual void Step(AssetImportEvent<RendererConfig>* token) final;

    private:
        EGID m_cameraEgid{};
        EntityDatabase* m_entityDb;
        AssetDatabase* m_assetDatabase;
    };
}