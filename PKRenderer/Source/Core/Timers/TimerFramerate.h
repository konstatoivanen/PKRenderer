#pragma once
#include "Core/Timers/ITimer.h"

namespace PK
{
    struct TimerFramerate : ITimer
    {
        uint64_t frameCount = 0;
        uint64_t framerate = 0;
        uint64_t framerateMin = (uint64_t)-1;
        uint64_t framerateMax = 0;
        uint64_t framerateAvg = 0;
        double elapsed = 0.0;

        void EndTimerScope() final;
    };
}
