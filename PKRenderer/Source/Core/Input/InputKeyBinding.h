#pragma once
#include <unordered_map>
#include "Core/Input/InputKey.h"

namespace PK
{
    typedef std::unordered_map<InputKey, std::string> InputKeyCommandBindingMap;

    struct CommandInputKeyBindingMap : public std::unordered_map<std::string, InputKey>
    {
        void TryGetKey(const char* command, InputKey* outKey) const;
    };
}