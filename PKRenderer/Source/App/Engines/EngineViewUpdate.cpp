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
        auto views = m_entityDb->Query<EntityViewRenderView>();

        for (auto& view : views)
        {
            auto time = view.time;
            auto input = view.input;
            
            time->info = ctx->time;
            // @TODO select input state on some view preference.
            
            if (ctx->input.lastDeviceState.state)
            {
                input->state = *ctx->input.lastDeviceState.state;
            }

            input->hotControlId = 0u;
            input->controlIdCounter = 1u;
        }
    }
}
