#pragma once
#include "Core/Services/IService.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "ECS/EntityDatabase.h"

namespace PK::Engines
{
	class EngineUpdateTransforms : public Core::Services::IService, 
		public Core::ControlFlow::IStepApplicationUpdateEngines
	{
		public:
			EngineUpdateTransforms(ECS::EntityDatabase* entityDb);
			virtual void OnApplicationUpdateEngines() final;
		
		private:
			ECS::EntityDatabase* m_entityDb = nullptr;
	};
}