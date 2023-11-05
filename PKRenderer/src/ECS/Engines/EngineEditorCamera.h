#pragma once
#include "Core/ApplicationConfig.h"
#include "Core/ConsoleCommandBinding.h"
#include "Core/Services/Input.h"
#include "Core/Services/Time.h"
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"

namespace PK::ECS::Engines
{
	class EngineEditorCamera : public Core::Services::IService, 
							   public Core::Services::IStep<Core::Services::Input>, 
							   public Core::Services::IStep<Core::TokenConsoleCommand>
	{
		public:
			EngineEditorCamera(Core::Services::Sequencer* sequencer, Core::Services::Time* time, const Core::ApplicationConfig* config);
			void Step(Core::Services::Input* input) final;
			void Step(Core::TokenConsoleCommand* token) final;

		private:
			Core::Services::Sequencer* m_sequencer = nullptr;
			Core::Services::Time* m_time = nullptr;
			const Core::ApplicationConfig* m_config = nullptr;
			Math::float3 m_position = Math::PK_FLOAT3_ZERO;
			Math::float3 m_eulerAngles = Math::PK_FLOAT3_ZERO;
			Math::quaternion m_rotation = Math::PK_QUATERNION_IDENTITY;
			Math::float3 m_smoothPosition = Math::PK_FLOAT3_ZERO;
			Math::quaternion m_smoothRotation = Math::PK_QUATERNION_IDENTITY;
			float m_fieldOfView = 60.0f;
			float m_zNear = 0.1f;
			float m_zFar = 250.0f;
			float m_moveSpeed = 5.0f;
			float m_moveSmoothing = 0.0f;
			float m_rotationSmoothing = 0.0f;
			float m_sensitivity = 1.0f;
	};
}