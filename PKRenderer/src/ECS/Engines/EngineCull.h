#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "ECS/EntityDatabase.h"
#include "ECS/Tokens/CullingTokens.h"

namespace PK::ECS::Engines
{
	class EngineCull : public Core::Services::IService, 
					   public Core::Services::IStep<Tokens::TokenCullFrustum>,
					   public Core::Services::IStep<Tokens::TokenCullCubeFaces>,
					   public Core::Services::IStep<Tokens::TokenCullCascades>
	{
		public:
			EngineCull(EntityDatabase* entityDb);
			void Step(Tokens::TokenCullFrustum* token) final;
			void Step(Tokens::TokenCullCubeFaces* token) final;
			void Step(Tokens::TokenCullCascades* token) final;

		private:
			EntityDatabase* m_entityDb = nullptr;
	};
}