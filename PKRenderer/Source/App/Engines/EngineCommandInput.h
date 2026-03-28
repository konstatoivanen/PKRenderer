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
        constexpr const static uint32_t LINE_COUNT = 32u;
        constexpr const static uint32_t LINE_LENGTH = 128u;

        EngineCommandInput(Sequencer* sequencer, InputKeyConfig* keyConfig);

        virtual void Step(IGUIRenderer* gui) final;
        virtual void OnStepFrameUpdate(FrameContext* ctx) final;
        virtual void Step(AssetImportEvent<InputKeyConfig>* evt) final;

    private:
        Sequencer* m_sequencer = nullptr;
        InputKeyCommandBindings m_inputKeyCommands;
        InputKey m_keyToggleConsole = InputKey::GraveAccent;
        
        FixedString<LINE_LENGTH> m_lineHint;
        FixedString<LINE_LENGTH> m_lines[LINE_COUNT];
        int32_t m_lineEdit = 0;
        int32_t m_lineHistory = 0;
        int32_t m_hintIndex = 0;
        bool m_cursorBlink = false;
        bool m_waitingInput = false;
    };
}
