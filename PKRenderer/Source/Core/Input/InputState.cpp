#include "PrecompiledHeader.h"
#include "InputState.h"

namespace PK
{
    float InputState::GetAxis(InputKey xneg, InputKey xpos) const
    {
        return GetKey(xpos) ? 1.0f : GetKey(xneg) ? -1.0f : 0.0f;
    }

    float2 InputState::GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos) const
    {
        return { GetAxis(xneg, xpos), GetAxis(yneg, ypos) };
    }

    float3 InputState::GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const
    {
        return { GetAxis(xneg, xpos), GetAxis(yneg, ypos), GetAxis(zneg, zpos) };
    }

    float InputState::GetAxisDown(InputKey xneg, InputKey xpos) const
    {
        return GetKeyDown(xpos) ? 1.0f : GetKeyDown(xneg) ? -1.0f : 0.0f;
    }

    float2 InputState::GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos) const
    {
        return { GetAxisDown(xneg, xpos), GetAxisDown(yneg, ypos) };
    }

    float3 InputState::GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const
    {
        return { GetAxisDown(xneg, xpos), GetAxisDown(yneg, ypos), GetAxisDown(zneg, zpos) };
    }

    bool InputState::ConsumeKeyDown(InputKey key)
    {
        return !keysConsumed[(int32_t)key].Exchange(true) && GetKeyDown(key);
    }

    bool InputState::ConsumeKeyUp(InputKey key)
    {
        return !keysConsumed[(int32_t)key].Exchange(true) && GetKeyUp(key);
    }

    bool InputState::ConsumeKey(InputKey key)
    {
        return !keysConsumed[(int32_t)key].Exchange(true) && GetKey(key);
    }

    float InputState::ConsumeAxis(InputKey xneg, InputKey xpos)
    {
        return ConsumeKey(xpos) ? 1.0f : ConsumeKey(xneg) ? -1.0f : 0.0f;
    }

    float2 InputState::ConsumeAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos)
    {
        return { ConsumeAxis(xneg, xpos), ConsumeAxis(yneg, ypos) };
    }

    float3 InputState::ConsumeAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos)
    {
        return { ConsumeAxis(xneg, xpos), ConsumeAxis(yneg, ypos), ConsumeAxis(zneg, zpos) };
    }

    float InputState::ConsumeAxisDown(InputKey xneg, InputKey xpos)
    {
        return ConsumeKeyDown(xpos) ? 1.0f : ConsumeKeyDown(xneg) ? -1.0f : 0.0f;
    }

    float2 InputState::ConsumeAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos)
    {
        return { ConsumeAxisDown(xneg, xpos), ConsumeAxisDown(yneg, ypos) };
    }

    float3 InputState::ConsumeAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos)
    {
        return { ConsumeAxisDown(xneg, xpos), ConsumeAxisDown(yneg, ypos), ConsumeAxisDown(zneg, zpos) };
    }

    void InputState::ConsumeAll()
    {
        keysConsumed.SetAll(true);
    }

    void InputState::SwapBuffers()
    {
        keysPrevious = keysCurrent;
        keysConsumed.SetAll(false);
        character = 0;
        cursorPositionDelta = PK_FLOAT2_ZERO;
    }
}
