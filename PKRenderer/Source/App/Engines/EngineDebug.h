#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Math/Math.h"
#include "Core/ECS/EGID.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "Core/Rendering/RenderingFwd.h"
#include "Core/Yaml/StructMacros.h"
#include "App/Renderer/RenderViewSettings.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)

namespace PK::App
{
    struct IGizmosRenderer;

    PK_YAML_ASSET_BEGIN(EngineDebugConfig, ".cfg")
        PK_YAML_MEMBER(float3, CameraStartPosition, PK_FLOAT3_ZERO)
        PK_YAML_MEMBER(float3, CameraStartRotation, PK_FLOAT3_ZERO)
        PK_YAML_MEMBER(float, CameraSpeed, 5.0f)
        PK_YAML_MEMBER(float, CameraLookSensitivity, 1.0f)
        PK_YAML_MEMBER(float, CameraMoveSmoothing, 0.0f)
        PK_YAML_MEMBER(float, CameraLookSmoothing, 0.0f)
        PK_YAML_MEMBER(float, CameraFov, 75.0f)
        PK_YAML_MEMBER(float, CameraZNear, 0.1f)
        PK_YAML_MEMBER(float, CameraZFar, 200.0f)
        PK_YAML_MEMBER(uint, LightCount, 0u)
        PK_YAML_MEMBER_STRUCT(RenderViewSettings, ViewSettings)
    PK_YAML_ASSET_END()

    // Dumping ground for all loose hooks that have not been implemented yet.
    class EngineDebug : 
        public IStepApplicationUpdateEngines<>,
        public IStep<IGizmosRenderer*>,
        public IStep<AssetImportEvent<EngineDebugConfig>*>
    {
    public:
        EngineDebug(AssetDatabase* assetDatabase, EntityDatabase* entityDb, MeshStaticCollection* baseMesh);
        virtual void OnApplicationUpdateEngines() final;
        virtual void Step(IGizmosRenderer* gui) final;
        virtual void Step(AssetImportEvent<EngineDebugConfig>* token) final;

    private:
        EGID m_cameraEgid{};
        EntityDatabase* m_entityDb;
        AssetDatabase* m_assetDatabase;
    };
}

PK_YAML_ASSET_ASSETDATABSE_INTERFACE(PK::App::EngineDebugConfig)