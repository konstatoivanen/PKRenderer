#include "PrecompiledHeader.h"
#include "TimerFramerate.h"

void PK::TimerFramerate::EndTimerScope()
{
    auto currentTimePoint = TimeHelpers::GetTimePointSecondsNow();
    auto deltaTime = (currentTimePoint - GetScopedTimePoint()).count();

    elapsed += deltaTime;

    ++frameCount;

    if (deltaTime > 0)
    {
        framerate = (uint64_t)(1.0 / deltaTime);
    }

    if (framerate < framerateMin)
    {
        framerateMin = framerate;
    }

    if (framerate > framerateMax)
    {
        framerateMax = framerate;
    }

    framerateAvg = (uint64_t)(frameCount / elapsed);
}
