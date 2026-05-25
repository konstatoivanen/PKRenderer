#pragma once
#include "Core/Math/Math.h"
#include "Core/Utilities/ArrayList.h"
#include "Core/CLI/CArguments.h"
#include "Core/Input/InputState.h"

namespace PK
{
    struct InputDevice;

    struct InputDeviceStatePair
    {
        const InputDevice* device = nullptr;
        InputState* state = nullptr;
    };

    struct InputDeviceFilePaths
    {
        const InputDevice* device = nullptr;
        CArgumentsInlineDefault paths;
    };

    struct InputStateCollection
    {
        constexpr const static uint32_t MAX_DEVICES = 5u;
        InputDeviceStatePair lastDeviceState;
        InputDeviceFilePaths droppedFilePaths;
        FixedList<InputDeviceStatePair, MAX_DEVICES> deviceStates;
    };
}
