#pragma once
#include "Core/Utilities/Ref.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/CLI/CArguments.h"
#include "Core/Input/InputKeyBinding.h"
#include "App/FrameStep.h"

namespace PK { struct Sequencer; }
namespace PK { struct InputState; }
namespace PK { struct InputKeyConfig; }

namespace PK::App
{
    struct IGUIRenderer;

    struct EngineCommandInput : 
        public IStep<IGUIRenderer*>,
        public IStepFrameUpdate<>,
        public IStep<AssetImportEvent<InputKeyConfig>*>
    {
        constexpr const static uint32_t COMMAND_HISTORY_SIZE = 16u;

        EngineCommandInput(Sequencer* sequencer, InputKeyConfig* keyConfig);

        virtual void Step(IGUIRenderer* gui) final;
        virtual void OnStepFrameUpdate(FrameContext* ctx) final;
        virtual void Step(AssetImportEvent<InputKeyConfig>* evt) final;

    private:
        Sequencer* m_sequencer = nullptr;
        InputKeyCommandBindings m_inputKeyCommands;
        FixedString128 m_commandInput;
        FixedString128 m_commandHistory[COMMAND_HISTORY_SIZE];
        InputKey m_keyBeginInput = InputKey::GraveAccent;
        uint32_t m_commandHistoryHead = 0u;
        int32_t m_commandHistoryView = 0;
        bool m_caretBlink = false;
        bool m_waitingInput = false;
    };
}
