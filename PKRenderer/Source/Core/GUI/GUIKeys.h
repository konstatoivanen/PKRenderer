#pragma once
#include "Core/Input/InputKey.h"

namespace PK
{
    enum class GUIKey : uint8_t
    {
        Enter,
        Exit,
        Shift,
        Control,
        Up,
        Down,
        Left,
        Right,
        Text_Backspace,
        Text_Delete,
        Text_Begin,
        Text_End,
        Text_Cut,
        Text_Copy,
        Text_Paste,
        Text_SelectAll
    };

    struct GUIKeys
    {
        InputTriplet Enter;
        InputTriplet Exit;
        InputTriplet Shift;
        InputTriplet Control;
        InputTriplet Up;
        InputTriplet Down;
        InputTriplet Left;
        InputTriplet Right;
        InputTriplet Text_Backspace;
        InputTriplet Text_Delete;
        InputTriplet Text_Begin;
        InputTriplet Text_End;
        InputTriplet Text_Cut;
        InputTriplet Text_Copy;
        InputTriplet Text_Paste;
        InputTriplet Text_SelectAll;
    };
}
