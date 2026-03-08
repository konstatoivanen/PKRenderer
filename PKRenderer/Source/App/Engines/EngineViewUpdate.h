#pragma once
#include "App/FrameStep.h"

namespace PK { struct EntityDatabase; }
namespace PK { struct Sequencer; }
namespace PK { struct InputState; }
namespace PK { struct TimeFrameInfo; }

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
