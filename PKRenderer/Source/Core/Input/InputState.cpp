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
}
