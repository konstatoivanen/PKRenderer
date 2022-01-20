#pragma once
#include "Core/Services/AssetDataBase.h"
#include "Core/ApplicationConfig.h"
#include "ECS/EntityDatabase.h"

namespace PK::ECS::Engines
{
	using namespace PK::Utilities;
	using namespace PK::Rendering::Objects;

	class EngineDebug : public IService, public ISimpleStep
	{
		public:
			EngineDebug(AssetDatabase* assetDatabase, EntityDatabase* entityDb, const ApplicationConfig* config);
			void Step(int condition) override;

		private:
			EntityDatabase* m_entityDb;
			AssetDatabase* m_assetDatabase;
			Ref<Mesh> m_virtualBaseMesh = nullptr;
	};
}