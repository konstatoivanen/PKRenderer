#pragma once
#include "Core/Rendering/Window.h"
#include "Core/Input/InputStateCollection.h"
#include "Core/Timers/TimeFrameInfo.h"
#include "Core/Timers/TimerFramerate.h"

namespace PK::App
{
    struct FrameContext
    {
        Window* window = nullptr;
        InputStateCollection input{};
        TimeFrameInfo time{};
        TimeFramerateInfo framerate{};
    };
}
