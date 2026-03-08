#pragma once
#include "App/FrameStep.h"

namespace PK { struct EntityDatabase; }

namespace PK::App
{
    class EngineUpdateTransforms : public IStepFrameUpdate<>
    {
    public:
        EngineUpdateTransforms(EntityDatabase* entityDb);
        virtual void OnStepFrameUpdate(FrameContext* ctx) final;

    private:
        EntityDatabase* m_entityDb = nullptr;
    };
}
