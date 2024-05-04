#include "PrecompiledHeader.h"
#include "InputDevice.h"

namespace PK::Core::Input
{
    using namespace PK::Math;
    using namespace PK::Core;

    float InputDevice::GetAxis(InputKey xneg, InputKey xpos) const
    {
        return GetKey(xpos) ? 1.0f : GetKey(xneg) ? -1.0f : 0.0f;
    }

    float2 InputDevice::GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos) const
    {
        return { GetAxis(xneg, xpos), GetAxis(yneg, ypos) };
    }

    float3 InputDevice::GetAxis(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const
    {
        return { GetAxis(xneg, xpos), GetAxis(yneg, ypos), GetAxis(zneg, zpos) };
    }

    float InputDevice::GetAxisDown(InputKey xneg, InputKey xpos) const
    {
        return GetKeyDown(xpos) ? 1.0f : GetKeyDown(xneg) ? -1.0f : 0.0f;
    }

    float2 InputDevice::GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos) const
    {
        return { GetAxisDown(xneg, xpos), GetAxisDown(yneg, ypos) };
    }

    float3 InputDevice::GetAxisDown(InputKey xneg, InputKey xpos, InputKey yneg, InputKey ypos, InputKey zneg, InputKey zpos) const
    {
        return { GetAxisDown(xneg, xpos), GetAxisDown(yneg, ypos), GetAxisDown(zneg, zpos) };
    }
}