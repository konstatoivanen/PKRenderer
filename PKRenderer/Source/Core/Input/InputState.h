#pragma once
#include <bitset>
#include "Core/Math/Math.h"
#include "Core/Input/InputKey.h"

namespace PK
{
    struct InputState
    {
        std::bitset<(int)InputKey::Count> keysCurrent;
        std::bitset<(int)InputKey::Count> keysPrevious;
        float2 cursorPosition = PK_FLOAT2_ZERO;
        float2 cursorPositionNormalized = PK_FLOAT2_ZERO;
        float2 cursorPositionDelta = PK_FLOAT2_ZERO;
        float2 cursorScroll = PK_FLOAT2_ZERO;

        float GetAxis(InputKey xneg, InputKey xpos)  const;
        float2 GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos)  const;
        float3 GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const;
        float GetAxisDown(InputKey xneg, InputKey xpos)  const;
        float2 GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos)  const;
        float3 GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const;

        bool GetKeyDown(InputKey key) const { return keysCurrent[(int)key] && !keysPrevious[(int)key]; }
        bool GetKeyUp(InputKey key) const { return !keysCurrent[(int)key] && keysPrevious[(int)key]; }
        bool GetKey(InputKey key) const { return keysCurrent[(int)key]; }
    };
}
