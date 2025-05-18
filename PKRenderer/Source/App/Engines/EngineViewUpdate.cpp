#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Input/InputState.h"
#include "Core/Timers/TimeFrameInfo.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/ControlFlow/ApplicationStep.h"
#include "App/ECS/EntityViewRenderView.h"
#include "EngineViewUpdate.h"

namespace PK::App
{
    EngineViewUpdate::EngineViewUpdate(Sequencer* sequencer, EntityDatabase* entityDb) :
        m_entityDb(entityDb),
        m_sequencer(sequencer)
    {
    }
    
    void EngineViewUpdate::Step(InputState* inputState)
    {
        auto views = m_entityDb->Query<EntityViewRenderView>((uint32_t)ENTITY_GROUPS::ACTIVE);

        for (auto i = 0u; i < views.count; ++i)
        {
            auto& input = views[i].input;
            input->state = *inputState;
            input->keysConsumed.Clear();
            input->hotControlId = 0u;
            input->controlIdCounter = 1u;
        }

        m_sequencer->Next(this, ApplicationStep::UpdateInput());
    }

    void EngineViewUpdate::Step(TimeFrameInfo* time)
    {
        auto views = m_entityDb->Query<EntityViewRenderView>((uint32_t)ENTITY_GROUPS::ACTIVE);

        for (auto i = 0u; i < views.count; ++i)
        {
            views[i].time->info = *time;
        }
    }
}
