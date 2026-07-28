#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Serialization/Serialize.h"

namespace PK
{
    struct CommandInputKeyBindingMap;
    struct InputKeyCommandBindings;
    template<> struct ISerializer<CommandInputKeyBindingMap>{ static void ReadVal(SerialNodeRead node, CommandInputKeyBindingMap* rhs);};
    template<> struct ISerializer<InputKeyCommandBindings>{ static void ReadVal(SerialNodeRead node, InputKeyCommandBindings* rhs);};
}
#endif
