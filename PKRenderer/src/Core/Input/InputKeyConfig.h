#pragma once
#include "Core/Yaml/ConvertInputKeyBindings.h"
#include "Core/Yaml/ConfigMacros.h"
#include "Core/Input/InputKeyBinding.h"

namespace PK::Core::Input
{
    PK_YAML_ASSET_BEGIN(InputKeyConfig, ".keycfg")
        PK_YAML_MEMBER(CommandInputKeyBindingMap, CommandInputKeys, CommandInputKeyBindingMap())
        PK_YAML_MEMBER(InputKeyCommandBindingMap, InputKeyCommands, InputKeyCommandBindingMap())
    PK_YAML_ASSET_END()
}

PK_YAML_ASSET_ASSETDATABSE_INTERFACE(PK::Core::Input::InputKeyConfig)
