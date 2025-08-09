#pragma once
#include "Core/Utilities/FastMap.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Utilities/MemoryBlock.h"
#include "Core/Utilities/Hash.h"
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
        MemoryBlock<char> memory;
        size_t count;

        InputKeyCommand* GetBindings() { return reinterpret_cast<InputKeyCommand*>(memory.GetData()); }
    };

    struct CommandInputKeyBindingMap : public FastMap<FixedString32, InputKey>
    {
        void TryGetKey(const char* command, InputKey* outKey) const;
    };
}