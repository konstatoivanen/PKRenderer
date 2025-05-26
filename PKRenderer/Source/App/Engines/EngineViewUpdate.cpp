#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Input/InputState.h"
#include "Core/Timers/TimeFrameInfo.h"
#include "Core/ControlFlow/Sequencer.h"
#include "App/ECS/EntityViewRenderView.h"
#include "App/FrameContext.h"
#include "EngineViewUpdate.h"

namespace PK::App
{
    EngineViewUpdate::EngineViewUpdate(Sequencer* sequencer, EntityDatabase* entityDb) :
        m_entityDb(entityDb),
        m_sequencer(sequencer)
    {
    }
    
    void EngineViewUpdate::OnStepFrameUpdate(FrameContext* ctx)
    {
        auto views = m_entityDb->Query<EntityViewRenderView>((uint32_t)ENTITY_GROUPS::ACTIVE);

        for (auto i = 0u; i < views.count; ++i)
        {
            auto& time = views[i].time;
            time->info = ctx->time;

            // @TODO select input state on some view preference.
            auto& input = views[i].input;
            input->state = ctx->input.globalState;
            input->keysConsumed.Clear();
            input->hotControlId = 0u;
            input->controlIdCounter = 1u;
        }
    }
}

