#pragma once
#include "Core/Math/Math.h"
#include "Core/Input/InputKey.h"

namespace PK
{
    struct InputState
    {
        constexpr const static uint32_t REPEAT_DELAY = 500u;

        InputKeyState keysCurrent;
        InputKeyState keysPrevious;
        InputKeyState keysRepeat;
        InputKeyState keysConsumed;
        uint32_t keyTimers[(uint32_t)InputKey::EnumCount]{};
        uint32_t character = 0u;
        float2 cursorPosition = PK_FLOAT2_ZERO;
        float2 cursorPositionDelta = PK_FLOAT2_ZERO;
        float2 cursorPositionNormalized = PK_FLOAT2_ZERO;
        float2 cursorScroll = PK_FLOAT2_ZERO;

        bool GetKeyDown(InputKey key) const;
        bool GetKeyUp(InputKey key) const;
        bool GetKey(InputKey key) const;
        bool GetKeyRepeat(InputKey key) const;
        uint32_t GetKeyTime(InputKey key) const;
        bool ConsumeKeyDown(InputKey key);
        bool ConsumeKeyUp(InputKey key);
        bool ConsumeKey(InputKey key);

        float GetAxis(InputKey xneg, InputKey xpos)  const;
        float2 GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos)  const;
        float3 GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const;
        float GetAxisDown(InputKey xneg, InputKey xpos)  const;
        float2 GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos)  const;
        float3 GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const;
        float ConsumeAxis(InputKey xneg, InputKey xpos);
        float2 ConsumeAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos);
        float3 ConsumeAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos);
        float ConsumeAxisDown(InputKey xneg, InputKey xpos);
        float2 ConsumeAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos);
        float3 ConsumeAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos);

        bool GetKeyDown(const InputShortcut& shortcut) const;
        bool GetKeyUp(const InputShortcut& shortcut) const;
        bool GetKey(const InputShortcut& shortcut) const;
        bool GetKeyRepeat(const InputShortcut& shortcut) const;
        bool ConsumeKeyDown(const InputShortcut& shortcut);
        bool ConsumeKeyUp(const InputShortcut& shortcut);
        bool ConsumeKey(const InputShortcut& shortcut);

        bool GetKeyDown(const InputTriplet& triplet) const;
        bool GetKeyUp(const InputTriplet& triplet) const;
        bool GetKey(const InputTriplet& triplet) const;
        bool GetKeyRepeat(const InputTriplet& triplet) const;
        bool ConsumeKeyDown(const InputTriplet& triplet);
        bool ConsumeKeyUp(const InputTriplet& triplet);
        bool ConsumeKey(const InputTriplet& triplet);

        float GetAxis(const InputTriplet& xneg, const InputTriplet& xpos)  const;
        float2 GetAxis(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos)  const;
        float3 GetAxis(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos, const InputTriplet& zneg, const InputTriplet& zpos) const;
        float GetAxisDown(const InputTriplet& xneg, const InputTriplet& xpos)  const;
        float2 GetAxisDown(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos)  const;
        float3 GetAxisDown(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos, const InputTriplet& zneg, const InputTriplet& zpos) const;
        float ConsumeAxis(const InputTriplet& xneg, const InputTriplet& xpos);
        float2 ConsumeAxis(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos);
        float3 ConsumeAxis(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos, const InputTriplet& zneg, const InputTriplet& zpos);
        float ConsumeAxisDown(const InputTriplet& xneg, const InputTriplet& xpos);
        float2 ConsumeAxisDown(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos);
        float3 ConsumeAxisDown(const InputTriplet& xneg, const InputTriplet& xpos, const InputTriplet& yneg, const InputTriplet& ypos, const InputTriplet& zneg, const InputTriplet& zpos);

        void ConsumeAll();

        void SetKey(InputKey key, bool isDown);
        void SetCursor(const float2& position, const float2& size);
        void SwapBuffers(uint32_t deltaMillis);
    };
}
