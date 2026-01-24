#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Rendering/Window.h"
#include "Core/Input/InputStateCollection.h"
#include "Core/Timers/TimeFrameInfo.h"
#include "Core/Timers/TimerFramerate.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct IArena);

namespace PK::App
{
    struct FrameContext
    {
        IArena* frameArena = nullptr;
        Window* window = nullptr;
        InputStateCollection input{};
        TimeFrameInfo time{};
        TimeFramerateInfo framerate{};
    };
}
