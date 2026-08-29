#pragma once
#include "Core/Base/Types/Ref.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/CLI/CArguments.h"
#include "Core/Input/InputKeyBinding.h"
#include "Core/Serialization/Config.h"
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
        public IStep<AssetImportEvent<Config<InputKeyConfig>>*>
    {
        constexpr const static uint32_t LINE_COUNT = 32u;
        constexpr const static uint32_t LINE_LENGTH = 128u;
        constexpr const static char* HISTORY_FILENAME = "Saved/ConsoleHistory.ini";

        EngineCommandInput(Sequencer* sequencer, InputKeyConfig* keyConfig);
        ~EngineCommandInput();

        virtual void Step(IGUIRenderer* gui) final;
        virtual void OnStepFrameUpdate(FrameContext* ctx) final;
        virtual void Step(AssetImportEvent<Config<InputKeyConfig>>* evt) final;

    private:
        bool ProcessConsoleInput(FrameContext* ctx);

        Sequencer* m_sequencer = nullptr;
        InputKeyCommandBindings m_inputKeyCommands;
        InputKey m_keyToggleConsole = InputKey::GraveAccent;
        
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
