#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "ECS/EntityDatabase.h"

namespace PK::ECS::Engines
{
	class EngineUpdateTransforms : public IService, public ISimpleStep
	{
		public:
			EngineUpdateTransforms(EntityDatabase* entityDb);
			void Step(int condition) override final;
		
		private:
			EntityDatabase* m_entityDb = nullptr;
	};
}