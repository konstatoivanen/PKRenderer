#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/NativeInterface.h"
#include "Core/Math/Math.h"
#include "Core/Input/InputKey.h"
#include "Core/Input/InputState.h"

namespace PK
{
    struct InputDevice : public NoCopy, public NativeInterface<InputDevice>
    {
        virtual const InputState* GetStatePtr() const = 0;
        virtual void UpdateBegin() = 0;
        virtual void UpdateEnd() = 0;
        virtual ~InputDevice() = default;

        float GetAxis(InputKey xneg, InputKey xpos) const { return GetStatePtr()->GetAxis(xneg, xpos); }
        float2 GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos) const { return GetStatePtr()->GetAxis(xneg, xpos, yneg, ypos); }
        float3 GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const { return GetStatePtr()->GetAxis(xneg, xpos, yneg, ypos, zneg, zpos); }
        float GetAxisDown(InputKey xneg, InputKey xpos) const { return GetStatePtr()->GetAxisDown(xneg, xpos); }
        float2 GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos) const { return GetStatePtr()->GetAxisDown(xneg, xpos, yneg, ypos); }
        float3 GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const { return GetStatePtr()->GetAxisDown(xneg, xpos, yneg, ypos, zneg, zpos); }
        inline bool GetKeyDown(InputKey key) const { return GetStatePtr()->GetKeyDown(key); }
        inline bool GetKeyUp(InputKey key) const { return GetStatePtr()->GetKeyUp(key); }
        inline bool GetKey(InputKey key) const { return GetStatePtr()->GetKey(key); }
        inline float2 GetCursorPosition() const { return GetStatePtr()->cursorPosition; }
        inline float2 GetCursorPositionNormalized() const { return GetStatePtr()->cursorPositionNormalized; }
        inline float2 GetCursorDelta() const { return GetStatePtr()->cursorPositionDelta; }
        inline float2 GetCursorScroll() const { return GetStatePtr()->cursorScroll; }
        inline float GetCursorDeltaX() const { return GetCursorDelta().x; }
        inline float GetCursorDeltaY() const { return GetCursorDelta().y; }
        inline float GetCursorX() const { return GetCursorPosition().x; }
        inline float GetCursorY() const { return GetCursorPosition().y; }
        inline float GetCursorNormalizedX() const { return GetCursorPositionNormalized().x; }
        inline float GetCursorNormalizedY() const { return GetCursorPositionNormalized().y; }
        inline float GetCursorScrollX() const { return GetCursorScroll().x; }
        inline float GetCursorScrollY() const { return GetCursorScroll().y; }
    };
}