#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/Base/Containers/ArrayList.h"
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
        HeapArray<char> memory;
        size_t count;

        InputKeyCommand* GetBindings() { return reinterpret_cast<InputKeyCommand*>(memory.GetData()); }
    };

    struct CommandInputKeyBindingMap : public HashMap<FixedString32, InputKey>
    {
        void TryGetKey(const char* command, InputKey* outKey) const;
    };
}
