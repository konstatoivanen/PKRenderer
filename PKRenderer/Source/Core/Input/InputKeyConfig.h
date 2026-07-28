#pragma once
#include "Core/Serialization/Config.h"
#include "Core/Input/InputKeyBinding.h"

namespace PK
{
    struct InputKeyConfig
    {
        CommandInputKeyBindingMap CommandInputKeys = CommandInputKeyBindingMap();
        InputKeyCommandBindings InputKeyCommands = {};
    };
}

template<> inline const char* PK::Asset::GetExtension<PK::Config<PK::InputKeyConfig>>() { return "*.keycfg"; }
