#pragma once
#include <unordered_map>
#include "Core/Utilities/FastMap.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Utilities/Hash.h"
#include "Core/Input/InputKey.h"

namespace PK
{
    struct InputKeyCommand
    {
        InputKey key;
        FixedString128 command;
    };

    struct InputKeyCommandBindings
    {
        MemoryBlock<InputKeyCommand> bindings;
        size_t count;
    };

    struct CommandInputKeyBindingMap : public std::unordered_map<std::string, InputKey>
    {
        void TryGetKey(const char* command, InputKey* outKey) const;
    };
}