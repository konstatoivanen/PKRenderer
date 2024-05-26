#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Utilities/Ref.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/CLI/CArguments.h"
#include "Core/Input/InputKeyBinding.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct InputDevice)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct InputKeyConfig)

namespace PK::App
{
    class EngineCommandInput : 
        public IStep<InputDevice*>,
        public IStep<AssetImportEvent<InputKeyConfig>*>
    {
    public:
        EngineCommandInput(Sequencer* sequencer, InputKeyConfig* keyConfig, const CArguments& arguments);

        virtual void Step(InputDevice* input) final;
        virtual void Step(AssetImportEvent<InputKeyConfig>* evt) final;

    private:
        Sequencer* m_sequencer = nullptr;
        InputKeyCommandBindingMap m_inputKeyCommands;
        InputKey m_keyBeginInput = InputKey::GraveAccent;
    };
}