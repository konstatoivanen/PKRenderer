#pragma once
#include "Core/Utilities/HashMap.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Utilities/FastBuffer.h"
#include "Core/Input/InputKey.h"

namespace PK
{
    struct InputKeyCommand
    {
        char* command;
        InputKey key;
    };

    struct InputKeyCommandBindings
    {
        FastBuffer<char> memory;
        size_t count;

        InputKeyCommand* GetBindings() { return reinterpret_cast<InputKeyCommand*>(memory.GetData()); }
    };

    struct CommandInputKeyBindingMap : public HashMap<FixedString32, InputKey>
    {
        void TryGetKey(const char* command, InputKey* outKey) const;
    };
}
