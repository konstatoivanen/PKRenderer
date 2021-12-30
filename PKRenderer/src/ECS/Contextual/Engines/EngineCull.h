#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "ECS/EntityDatabase.h"
#include "ECS/Contextual/Tokens/CullingTokens.h"

namespace PK::ECS::Engines
{
	using namespace PK::ECS::Tokens;
	using namespace PK::Math;
	using namespace PK::Rendering::Structs;
	using namespace PK::Core::Services;

	class EngineCull : public IService, 
					   public IStep<TokenCullFrustum>,
					   public IStep<TokenCullCubeFaces>,
					   public IStep<TokenCullCascades>
	{
		public:
			EngineCull(EntityDatabase* entityDb);
			void Step(TokenCullFrustum* token) override final;
			void Step(TokenCullCubeFaces* token) override final;
			void Step(TokenCullCascades* token) override final;

		private:
			EntityDatabase* m_entityDb = nullptr;
	};
}