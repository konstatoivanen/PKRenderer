#pragma once
#include "Core/Input/InputKeyBinding.h"

namespace PK
{
    struct InputKeyConfig
    {
        CommandInputKeyBindingMap CommandInputKeys = CommandInputKeyBindingMap();
        InputKeyCommandBindings InputKeyCommands = {};
    };
}
