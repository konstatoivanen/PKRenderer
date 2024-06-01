#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "ConfigMacros.h"

void YAML::IYamlConfig::YamlLoadFromFile(const std::string& filepath)
{
    auto properties = YAML::LoadFile(filepath);
    PK_THROW_ASSERT(properties, "Failed to open yaml file at: %s", filepath.c_str());
    YamlParse(properties);
}
