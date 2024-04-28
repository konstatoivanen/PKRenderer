#pragma once
#include "Core/Services/IService.h"
#include "Core/ControlFlow/Sequencer.h"
#include "ECS/EntityDatabase.h"
#include "Rendering/EntityCulling.h"

namespace PK::Engines
{
	class EngineEntityCull : public Core::Services::IService, 
		public Core::ControlFlow::IStep<Rendering::RequestEntityCullFrustum*>,
		public Core::ControlFlow::IStep<Rendering::RequestEntityCullCubeFaces*>,
		public Core::ControlFlow::IStep<Rendering::RequestEntityCullCascades*>
	{
		public:
			EngineEntityCull(ECS::EntityDatabase* entityDb);
			virtual void Step(Rendering::RequestEntityCullFrustum* request) final;
			virtual void Step(Rendering::RequestEntityCullCubeFaces* request) final;
			virtual void Step(Rendering::RequestEntityCullCascades* request) final;

		private:
			ECS::EntityDatabase* m_entityDb = nullptr;

			Rendering::CulledEntityInfoList m_synchronousResults;
	};
}