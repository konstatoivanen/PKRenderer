#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "App/FrameStep.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct InputState)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct TimeFrameInfo)

namespace PK::App
{
    class EngineViewUpdate : public IStepFrameUpdate<>
    {
    public:
        EngineViewUpdate(Sequencer* sequencer, EntityDatabase* entityDb);
        virtual void OnStepFrameUpdate(FrameContext* ctx) final;

    private:
        EntityDatabase* m_entityDb;
        Sequencer* m_sequencer;
    };
}
