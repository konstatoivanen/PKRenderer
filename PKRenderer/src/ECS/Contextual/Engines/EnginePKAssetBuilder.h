#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "Core/ConsoleCommandBinding.h"
#include "Core/ApplicationConfig.h"

namespace PK::ECS::Engines
{
	using namespace PK::Math;
	using namespace PK::Rendering::Structs;
	using namespace PK::Core;
	using namespace PK::Core::Services;

	class EnginePKAssetBuilder : public IService, public IStep<ConsoleCommandToken>
	{
		public:
			EnginePKAssetBuilder(const ApplicationArguments& arguments);
			void Step(ConsoleCommandToken* token) override final;

		private:
			std::wstring m_executablePath;
			std::vector<wchar_t> m_executableArguments;
	};
}