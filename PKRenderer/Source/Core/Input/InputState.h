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
        uint32_t keyTimers[(uint32_t)InputKey::Count]{};
        uint32_t character = 0u;
        float2 cursorPosition = PK_FLOAT2_ZERO;
        float2 cursorPositionDelta = PK_FLOAT2_ZERO;
        float2 cursorPositionNormalized = PK_FLOAT2_ZERO;
        float2 cursorScroll = PK_FLOAT2_ZERO;

        void SetKey(InputKey key, bool isDown);

        bool GetKeyDown(InputKey key) const;
        bool GetKeyUp(InputKey key) const;
        bool GetKey(InputKey key) const;
        bool GetKeyRepeat(InputKey key) const;
        uint32_t GetKeyTime(InputKey key) const;
        float GetAxis(InputKey xneg, InputKey xpos)  const;
        float2 GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos)  const;
        float3 GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const;
        float GetAxisDown(InputKey xneg, InputKey xpos)  const;
        float2 GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos)  const;
        float3 GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const;

        bool ConsumeKeyDown(InputKey key);
        bool ConsumeKeyUp(InputKey key);
        bool ConsumeKey(InputKey key);
        float ConsumeAxis(InputKey xneg, InputKey xpos);
        float2 ConsumeAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos);
        float3 ConsumeAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos);
        float ConsumeAxisDown(InputKey xneg, InputKey xpos);
        float2 ConsumeAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos);
        float3 ConsumeAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos);

        void ConsumeAll();
        void SwapBuffers(uint32_t deltaMillis);
    };
}
