#include "PrecompiledHeader.h"
#include "CommandConfig.h"

namespace PK::Core
{
	CommandConfig::CommandConfig() { values = { &Commands }; }

	void CommandConfig::Import(const char* filepath)
	{
		Load(filepath);
	}

	template<>
	bool Services::AssetImporters::IsValidExtension<CommandConfig>(const std::filesystem::path& extension) { return extension.compare(".keycfg") == 0; }

	template<>
	Utilities::Ref<CommandConfig> Services::AssetImporters::Create() { return Utilities::CreateRef<CommandConfig>(); }
}