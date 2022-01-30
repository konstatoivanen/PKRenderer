#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "Core/ConsoleCommandBinding.h"
#include "Core/ApplicationConfig.h"

namespace PK::ECS::Engines
{
	class EnginePKAssetBuilder : public Core::Services::IService, public Core::Services::IStep<Core::ConsoleCommandToken>
	{
		public:
			EnginePKAssetBuilder(const Core::ApplicationArguments& arguments);
			void Step(Core::ConsoleCommandToken* token) override final;

		private:
			std::wstring m_executablePath;
			std::vector<wchar_t> m_executableArguments;
	};
}