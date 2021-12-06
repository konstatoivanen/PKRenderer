#pragma once
#include "PrecompiledHeader.h"
#include "Core/YamlSerializers.h"

namespace PK::Core
{
	using namespace YAML;

	struct CommandConfig : YamlValueList, public Asset
	{
		BoxedValue<ConsoleCommandBindList> Commands = BoxedValue<ConsoleCommandBindList>("Commands", ConsoleCommandBindList());
		CommandConfig();
		void Import(const char* filepath) override final;
	};
}