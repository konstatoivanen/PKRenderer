#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Utilities/Ref.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/CLI/CArguments.h"
#include "Core/Input/InputKeyBinding.h"
#include "App/FrameStep.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct InputState)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct InputKeyConfig)

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
