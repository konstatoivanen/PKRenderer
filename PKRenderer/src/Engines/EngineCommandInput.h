#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/Ref.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/CLI/CArguments.h"
#include "Core/Input/InputKeyBinding.h"
#include "Core/IService.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::ControlFlow, class Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Input, struct InputDevice)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Input, struct InputKeyConfig)

namespace PK::Engines
{
    class EngineCommandInput : public Core::IService,
        public Core::ControlFlow::IStep<Core::Input::InputDevice*>,
        public Core::ControlFlow::IStep<Core::Assets::AssetImportEvent<Core::Input::InputKeyConfig>*>
    {
    public:
        EngineCommandInput(Core::ControlFlow::Sequencer* sequencer,
            Core::Input::InputKeyConfig* keyConfig,
            const Core::CLI::CArguments& arguments);

        virtual void Step(Core::Input::InputDevice* input) final;
        virtual void Step(Core::Assets::AssetImportEvent<Core::Input::InputKeyConfig>* evt) final;

    private:
        Core::ControlFlow::Sequencer* m_sequencer = nullptr;
        Core::Input::InputKeyCommandBindingMap m_inputKeyCommands;
        Core::Input::InputKey m_keyBeginInput = Core::Input::InputKey::GraveAccent;
    };
}