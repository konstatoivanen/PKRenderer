#pragma once
#include "Core/Math/Math.h"
#include "Core/ECS/EGID.h"
#include "Core/Rendering/RenderingFwd.h"
#include "Core/Yaml/Config.h"
#include "App/FrameStep.h"
#include "App/Renderer/RenderViewSettings.h"

namespace PK { class AssetDatabase; }
namespace PK { struct EntityDatabase; }

namespace PK::App
{
    struct IGizmosRenderer;

    struct EngineDebugConfig 
    {
        float3 CameraStartPosition = PK_FLOAT3_ZERO;
        float3 CameraStartRotation = PK_FLOAT3_ZERO;
        float CameraSpeed = 5.0f;
        float CameraLookSensitivity = 1.0f;
        float CameraMoveSmoothing = 0.0f;
        float CameraLookSmoothing = 0.0f;
        float CameraFov = 75.0f;
        float CameraZNear = 0.1f;
        float CameraZFar = 200.0f;
        uint LightCount = 0u;
        RenderViewSettings ViewSettings = {};
    };

    // Dumping ground for all loose hooks that have not been implemented yet.
    class EngineDebug : 
        public IStepFrameUpdate<>,
        public IStep<IGizmosRenderer*>,
        public IStep<AssetImportEvent<Config<EngineDebugConfig>>*>
    {
    public:
        EngineDebug(AssetDatabase* assetDatabase, EntityDatabase* entityDb, MeshStaticAllocator* meshAllocator);
        virtual void OnStepFrameUpdate(FrameContext* ctx) final;
        virtual void Step(IGizmosRenderer* gui) final;
        virtual void Step(AssetImportEvent<Config<EngineDebugConfig>>* token) final;

    private:
        EGID m_cameraEgid{};
        EntityDatabase* m_entityDb;
        AssetDatabase* m_assetDatabase;
    };
}

template<> inline const char* PK::Asset::GetExtension<PK::Config<PK::App::EngineDebugConfig>>() { return "*.cfg"; }

