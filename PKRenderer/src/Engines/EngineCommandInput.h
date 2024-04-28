#pragma once
#include "Utilities/Ref.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/CLI/CArguments.h"
#include "Core/Input/InputDevice.h"
#include "Core/Input/InputKeyConfig.h"
#include "Core/Services/Time.h"
#include "ECS/EntityDatabase.h"

namespace PK::Engines
{
	class EngineCommandInput : public Core::Services::IService, 
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