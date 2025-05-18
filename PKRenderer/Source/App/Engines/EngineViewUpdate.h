#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct InputState)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct TimeFrameInfo)

namespace PK::App
{
    class EngineViewUpdate :
        public IStep<InputState*>,
        public IStep<TimeFrameInfo*>
    {
    public:
        EngineViewUpdate(Sequencer* sequencer, EntityDatabase* entityDb);
        virtual void Step(InputState* inputState) final;
        virtual void Step(TimeFrameInfo* time) final;

    private:
        EntityDatabase* m_entityDb;
        Sequencer* m_sequencer;
    };
}
