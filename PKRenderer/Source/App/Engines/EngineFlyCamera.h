#pragma once
#include "Core/Assets/AssetImportEvent.h"
#include "Core/Serialization/Config.h"
#include "App/InputConfig.h"
#include "App/FrameStep.h"

namespace PK { struct EntityDatabase; }

namespace PK::App
{
    class EngineFlyCamera : 
        public IStepFrameUpdate<>,
        public IStep<AssetImportEvent<Config<InputConfig>>*>
    {
    public:
        EngineFlyCamera(EntityDatabase* entityDb, InputConfig* keyConfig);
        virtual void OnStepFrameUpdate(FrameContext* ctx) final;
        virtual void Step(AssetImportEvent<Config<InputConfig>>* evt) final { m_keys = evt->asset->FlyCamera; }

        void TransformsLog() const;
        void TransformsReset();

    private:
        EntityDatabase* m_entityDb;
        InputFlyCamera m_keys{};
    };
}
