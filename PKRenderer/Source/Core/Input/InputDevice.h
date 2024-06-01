#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/NativeInterface.h"
#include "Core/Math/Math.h"
#include "Core/Input/InputKey.h"

namespace PK
{
    struct InputDevice : public NoCopy, public NativeInterface<InputDevice>
    {
        virtual bool GetKeyDown(InputKey key) const = 0;
        virtual bool GetKeyUp(InputKey key) const = 0;
        virtual bool GetKey(InputKey key) const = 0;
        virtual float2 GetCursorPosition() const = 0;
        virtual float2 GetCursorPositionNormalized() const = 0;
        virtual float2 GetCursorDelta() const = 0;
        virtual float2 GetCursorScroll() const = 0;
        virtual void UpdateBegin() = 0;
        virtual void UpdateEnd() = 0;
        virtual ~InputDevice() = default;

        float GetAxis(InputKey xneg, InputKey xpos)  const;
        float2 GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos)  const;
        float3 GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const;
        float GetAxisDown(InputKey xneg, InputKey xpos)  const;
        float2 GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos)  const;
        float3 GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const;

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