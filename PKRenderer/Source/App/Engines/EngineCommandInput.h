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
    class EngineCommandInput : 
        public IStepFrameUpdate<>,
        public IStep<AssetImportEvent<InputKeyConfig>*>
    {
    public:
        EngineCommandInput(Sequencer* sequencer, InputKeyConfig* keyConfig);

        virtual void OnStepFrameUpdate(FrameContext* ctx) final;
        virtual void Step(AssetImportEvent<InputKeyConfig>* evt) final;

    private:
        Sequencer* m_sequencer = nullptr;
        InputKeyCommandBindings m_inputKeyCommands;
        InputKey m_keyBeginInput = InputKey::GraveAccent;
    };
}
