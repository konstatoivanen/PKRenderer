#pragma once
#include <cstdint>

namespace PK
{
    struct TimeFrameInfo
    {
        uint64_t frameIndex = 0;
        uint64_t framerate = 0;
        uint64_t framerateMin = (uint64_t)-1;
        uint64_t framerateMax = 0;
        uint64_t framerateAvg = 0;
        uint64_t framerateFixed = 0;
        double timeScale = 0.0;
        double time = 0.0;
        double unscaledTime = 0.0;
        double deltaTime = 0.0;
        double unscaledDeltaTime = 0.0;
        double smoothDeltaTime = 0.0;
        double unscaledDeltaTimeFixed = 0.0;
    };
}