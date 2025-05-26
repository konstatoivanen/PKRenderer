#pragma once
#include "Core/Math/Math.h"
#include "Core/Utilities/FixedList.h"
#include "Core/Input/InputState.h"

namespace PK
{
    struct InputDevice;

    struct InputDeviceStatePair
    {
        const InputDevice* device;
        InputState state;
    };

    struct InputStateCollection
    {
        constexpr const static uint32_t MAX_DEVICES = 5u;
        InputState globalState;
        InputDeviceStatePair lastDeviceState;
        FixedList<InputDeviceStatePair, MAX_DEVICES> deviceStates;
    };
}
