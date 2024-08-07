#pragma once
#include <cstdint>

namespace PK
{
    struct TimeFrameInfo
    {
        uint64_t frameIndex = 0;
        double timeScale = 0.0;
        double time = 0.0;
        double unscaledTime = 0.0;
        double deltaTime = 0.0;
        double unscaledDeltaTime = 0.0;
        double smoothDeltaTime = 0.0;
    };

    struct TimeFramerateInfo
    {
        uint64_t framerate = 0;
        uint64_t framerateMin = 0;
        uint64_t framerateMax = 0;
        uint64_t framerateAvg = 0;
        double frameMs = 0.0;
    };
}