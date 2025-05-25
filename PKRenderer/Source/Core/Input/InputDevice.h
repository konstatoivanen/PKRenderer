#pragma once
#include "Core/Input/InputKey.h"
#include "Core/Math/MathFwd.h"

namespace PK
{
    struct InputDevice;

    struct InputHandler
    {
        virtual ~InputHandler() = default;
        virtual void InputHandler_OnPoll() = 0;
        virtual void InputHandler_OnPoll(InputDevice* device) = 0;
        virtual void InputHandler_OnKey(InputDevice* device, InputKey key, bool isDown) = 0;
        virtual void InputHandler_OnMouseMoved(InputDevice* device, const float2& position, const float2& size) = 0;
        virtual void InputHandler_OnScroll(InputDevice* device, uint32_t axis, float offset) = 0;
        virtual void InputHandler_OnCharacter(InputDevice* device, uint32_t character) = 0;
        virtual void InputHandler_OnDrop(InputDevice* device, const char* const* paths, uint32_t count) = 0;
        virtual void InputHandler_OnConnect(InputDevice* device) = 0;
        virtual void InputHandler_OnDisconnect(InputDevice* device) = 0;
    };

    struct InputDevice
    {
        virtual ~InputDevice() = default;
        virtual InputType GetInputType() = 0;
        virtual float2 GetInputCursorPosition() = 0;
        virtual const InputKeyState& GetInputKeyState() = 0;
        virtual float GetInputAnalogAxis(InputKey neg, InputKey pos) = 0;
        virtual void SetUseRawInput(bool value) = 0;
    };
}
