#pragma once
#include "Core/Math/Math.h"
#include "Core/Utilities/FixedList.h"
#include "Core/CLI/CArguments.h"
#include "Core/Input/InputState.h"

namespace PK
{
    struct InputDevice;

    struct InputDeviceStatePair
    {
        const InputDevice* device;
        InputState state;
    };

    struct InputDeviceFilePaths
    {
        const InputDevice* device;
        CArgumentsInlineDefault paths;
    };

    struct InputStateCollection
    {
        constexpr const static uint32_t MAX_DEVICES = 5u;
        InputState globalState;
        InputDeviceStatePair lastDeviceState;
        InputDeviceFilePaths droppedFilePaths;
        FixedList<InputDeviceStatePair, MAX_DEVICES> deviceStates;
    };
}
