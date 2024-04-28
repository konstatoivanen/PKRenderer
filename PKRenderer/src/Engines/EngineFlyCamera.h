#pragma once
#include "Core/ApplicationConfig.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/Input/InputDevice.h"
#include "Core/Input/InputKeyConfig.h"
#include "Core/Input/InputKeyStructMacros.h"
#include "Core/Services/IService.h"
#include "Core/Services/Time.h"
#include "ECS/EntityDatabase.h"

namespace PK::Engines
{
	PK_INPUTKEY_STRUCT_BEGIN(EngineFlyCameraInputKeys)
		PK_INPUTKEY_STRUCT_MEMBER(Move,	  Forward,			W)
		PK_INPUTKEY_STRUCT_MEMBER(Move,	  Backward,			S)
		PK_INPUTKEY_STRUCT_MEMBER(Move,	  Left,				A)
		PK_INPUTKEY_STRUCT_MEMBER(Move,	  Right,			D)
		PK_INPUTKEY_STRUCT_MEMBER(Move,	  Up,				Q)
		PK_INPUTKEY_STRUCT_MEMBER(Move,	  Down,				E)
		PK_INPUTKEY_STRUCT_MEMBER(Camera, LookDrag,			Mouse1)
		PK_INPUTKEY_STRUCT_MEMBER(Camera, SpeedUp,			MouseScrollUp)
		PK_INPUTKEY_STRUCT_MEMBER(Camera, SpeedDown,		MouseScrollDown)
		PK_INPUTKEY_STRUCT_MEMBER(Camera, FovAdd,			MouseScrollDown)
		PK_INPUTKEY_STRUCT_MEMBER(Camera, FovSub,			MouseScrollUp)
		PK_INPUTKEY_STRUCT_MEMBER(Camera, FovControl,		LeftControl)
		PK_INPUTKEY_STRUCT_MEMBER(Camera, ResetSmoothing,	LeftShift)
		PK_INPUTKEY_STRUCT_MEMBER(Camera, DollyZoom,		LeftShift)
	PK_INPUTKEY_STRUCT_END()		

	class EngineFlyCamera : public Core::Services::IService, 
		public Core::ControlFlow::IStep<Core::Input::InputDevice*>,
		public Core::ControlFlow::IStep<Core::Services::TimeFrameInfo*>,
		public Core::ControlFlow::IStep<Core::Assets::AssetImportEvent<Core::Input::InputKeyConfig>*>
	{
		public:
			EngineFlyCamera(class ECS::EntityDatabase* entityDb, Core::Input::InputKeyConfig* keyConfig);
			virtual void Step(Core::Input::InputDevice* input) final;
			virtual void Step(Core::Services::TimeFrameInfo* time) final { m_timeFrameInfo = *time; }
			virtual void Step(Core::Assets::AssetImportEvent<Core::Input::InputKeyConfig>* evt) final { m_keys.SetKeysFrom(evt->asset); }

			void TransformsLog() const;
			void TransformsReset();

		private:
			ECS::EntityDatabase* m_entityDb;
			Core::Services::TimeFrameInfo m_timeFrameInfo{};
			EngineFlyCameraInputKeys m_keys{};
	};
}