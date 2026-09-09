#pragma once
#include "Core/Input/InputKey.h"
#include "Core/Input/InputKeyBindings.h"
#include "Core/Input/InputKeyCommands.h"
#include "Core/GUI/GUIKeys.h"

namespace PK::App
{
    struct InputFlyCamera
    {
        InputTriplet Forward = InputKey::W;
        InputTriplet Backward = InputKey::S;
        InputTriplet Left = InputKey::A;
        InputTriplet Right = InputKey::D;
        InputTriplet Up = InputKey::Q;
        InputTriplet Down = InputKey::E;
        InputTriplet LookDrag = InputKey::Mouse2;
        InputTriplet SpeedUp = InputKey::MouseScrollUp;
        InputTriplet SpeedDown = InputKey::MouseScrollDown;
        InputTriplet FovAdd = InputKey::MouseScrollDown;
        InputTriplet FovSub = InputKey::MouseScrollUp;
        InputTriplet FovControl = InputKey::LeftControl;
        InputTriplet ResetSmoothing = InputKey::LeftShift;
        InputTriplet DollyZoom = InputKey::LeftShift;
    };

    struct InputConfig
    {
        InputTriplet OpenConsole = InputKey::GraveAccent;
        InputFlyCamera FlyCamera{};
        GUIKeys GUIKeys{};
        InputKeyBindings KeyBindings;
        InputKeyCommands KeyCommands;
    };
}