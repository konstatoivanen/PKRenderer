#pragma once
#include "Core/Services/AssetDatabase.h"
#include "Core/YamlSerializers.h"

namespace PK::Core
{
	struct CommandConfig : YAML::YamlValueList, public Core::Services::Asset, public Core::Services::IAssetImportSimple
	{
		YAML::BoxedValue<ConsoleCommandBindList> Commands = YAML::BoxedValue<ConsoleCommandBindList>("Commands", ConsoleCommandBindList());
		CommandConfig();
		void Import(const char* filepath) override final;
	};
}