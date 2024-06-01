#pragma once
#include "Core/Timers/ITimer.h"

namespace PK
{
    struct TimerFrameRunner : ITimer
    {
        uint64_t frameIndex = 0;
        double timeScale = 1.0;
        double time = 0.0;
        double unscaledTime = 0.0;
        double deltaTime = 0.0;
        double unscaledDeltaTime = 0.0;
        double smoothDeltaTime = 0.0;

        void EndTimerScope() final;
    };
}
