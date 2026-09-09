#pragma once
#include "Core/Base/Containers/Mask.h"

namespace PK
{
    enum class InputKey : uint8_t
    {
        None,
        Mouse1,
        Mouse2,
        Mouse3,
        Mouse4,
        Mouse5,
        Mouse6,
        Mouse7,
        Mouse8,
        MouseScrollDown,
        MouseScrollUp,
        MouseScrollLeft,
        MouseScrollRight,
        MouseHover,
        Space,
        Apostrophe,
        Comma,
        Minus,
        Period,
        Slash,
        Alpha0,
        Alpha1,
        Alpha2,
        Alpha3,
        Alpha4,
        Alpha5,
        Alpha6,
        Alpha7,
        Alpha8,
        Alpha9,
        Semicolon,
        Equal,
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        LeftBracket,
        BackSlash,
        RightBracket,
        GraveAccent,
        World1,
        World2,
        Escape,
        Enter,
        Tab,
        Backspace,
        Insert,
        Del,
        Right,
        Left,
        Down,
        Up,
        PageUp,
        PageDown,
        Home,
        End,
        CapsLock,
        ScrollLock,
        NumLock,
        PrintScreen,
        Pause,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
        F21,
        F22,
        F23,
        F24,
        F25,
        KeyPad0,
        KeyPad1,
        KeyPad2,
        KeyPad3,
        KeyPad4,
        KeyPad5,
        KeyPad6,
        KeyPad7,
        KeyPad8,
        KeyPad9,
        KeyPadDecimal,
        KeyPadDivide,
        KeyPadMultiply,
        KeyPadSubtract,
        KeyPadAdd,
        KeyPadEnter,
        KeyPadEqual,
        LeftShift,
        LeftControl,
        LeftAlt,
        LeftSuper,
        RightShift,
        RightControl,
        RightAlt,
        RightSuper,
        Menu,
        Count
    };

    enum class InputType : uint8_t
    {
        KeyboardAndMouse,
        XBOX,
        DS5
    };

    typedef FixedMask<(uint32_t)InputKey::Count> InputKeyState;

    struct InputShortcut
    {
        InputKey First;
        InputKey Second;
        constexpr InputShortcut() = default;
        constexpr InputShortcut(InputKey key0) : First(key0) {}
        constexpr InputShortcut(InputKey key0, InputKey key1) : First(key0), Second(key1) {}
    };

    struct InputTriplet
    {
        InputShortcut Primary;
        InputShortcut Secondary;
        InputShortcut Tertiary;
        constexpr InputTriplet() = default;
        constexpr InputTriplet(InputKey key0) : Primary(key0) {}
        constexpr InputTriplet(InputKey key0, InputKey key1) : Primary(key0), Secondary(key1) {}
        constexpr InputTriplet(InputKey key0, InputKey key1, InputKey key2) : Primary(key0), Secondary(key1), Tertiary(key2) {}
        constexpr InputTriplet(InputShortcut key0) : Primary(key0) {}
        constexpr InputTriplet(InputShortcut key0, InputShortcut key1) : Primary(key0), Secondary(key1) {}
        constexpr InputTriplet(InputShortcut key0, InputShortcut key1, InputShortcut key2) : Primary(key0), Secondary(key1), Tertiary(key2) {}
    };
}
