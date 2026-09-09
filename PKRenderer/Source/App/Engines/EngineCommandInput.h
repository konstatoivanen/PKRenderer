#pragma once
#include "Core/Base/Containers/FixedString.h"
#include "Core/Base/Types/Ref.h"
#include "Core/CLI/CArguments.h"
#include "Core/Input/InputKey.h"
#include "App/FrameStep.h"

namespace PK { struct Sequencer; }
namespace PK { struct InputState; }
namespace PK { struct InputKeyCommands; }
namespace PK { struct GUI; }

namespace PK::App
{
    struct EngineCommandInput : 
        public IStep<GUI*>,
        public IStepFrameUpdate<>
    {
        constexpr const static uint32_t LINE_COUNT = 32u;
        constexpr const static uint32_t LINE_LENGTH = 128u;
        constexpr const static char* HISTORY_FILENAME = "Saved/ConsoleHistory.ini";

        EngineCommandInput(Sequencer* sequencer, const InputKeyCommands* commands, const InputTriplet& toggleConsole);
        ~EngineCommandInput();

        virtual void Step(GUI* gui) final;
        virtual void OnStepFrameUpdate(FrameContext* ctx) final;

    private:
        bool ProcessConsoleInput(FrameContext* ctx);

        const InputKeyCommands* m_inputKeyCommands;
        const InputTriplet m_keyToggleConsole;
        Sequencer* m_sequencer = nullptr;
        
        FixedString<LINE_LENGTH> m_lineHint;
        FixedString<LINE_LENGTH> m_lines[LINE_COUNT];
        uint32_t m_lineEdit = 0;
        uint32_t m_lineHistory = 0;
        int32_t m_hintIndex = 0;
        uint32_t m_caretTimer = 0u;
        bool m_waitingInput = false;
        bool m_isElevated = false;
    };
}
