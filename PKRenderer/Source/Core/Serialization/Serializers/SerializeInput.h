#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Serialization/Serialize.h"

namespace PK
{
    struct InputShortcut;
    struct InputTriplet;
    struct InputKeyBindings;
    struct InputKeyCommands;
    
    template<> struct ISerializer<InputShortcut>
    {
        static void ReadVal(SerialNodeRead node, InputShortcut* rhs);
        static void WriteVal(SerialNodeWrite node, InputShortcut const* rhs);
    };

    template<> struct ISerializer<InputTriplet>
    {
        static void ReadVal(SerialNodeRead node, InputTriplet* rhs);
        static void WriteVal(SerialNodeWrite node, InputTriplet const* rhs);
    };

    template<> struct ISerializer<InputKeyBindings>
    {
        static void ReadVal(SerialNodeRead node, InputKeyBindings* rhs);
        static void WriteVal(SerialNodeWrite node, InputKeyBindings const* rhs);
    };

    template<> struct ISerializer<InputKeyCommands>
    {
        static void ReadVal(SerialNodeRead node, InputKeyCommands* rhs);
        static void WriteVal(SerialNodeWrite node, InputKeyCommands const* rhs);
    };
}
#endif
