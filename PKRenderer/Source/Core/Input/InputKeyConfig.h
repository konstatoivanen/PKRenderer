#pragma once
#include "Core/Yaml/StructMacros.h"
#include "Core/Input/InputKeyBinding.h"

namespace PK
{
    PK_YAML_ASSET_BEGIN(InputKeyConfig, ".keycfg")
        PK_YAML_MEMBER(CommandInputKeyBindingMap, CommandInputKeys, CommandInputKeyBindingMap())
        PK_YAML_MEMBER(InputKeyCommandBindings, InputKeyCommands, {})
    PK_YAML_ASSET_END()
}

PK_YAML_ASSET_ASSETDATABSE_INTERFACE(PK::InputKeyConfig)
