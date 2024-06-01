#include "PrecompiledHeader.h"
#include "TimerFrameRunner.h"

void PK::TimerFrameRunner::EndTimerScope()
{
    auto currentTimePoint = TimeHelpers::GetTimePointSecondsNow();
    unscaledDeltaTime = (currentTimePoint - GetScopedTimePoint()).count();
    deltaTime = unscaledDeltaTime * timeScale;
    unscaledTime += unscaledDeltaTime;
    time += deltaTime;
    smoothDeltaTime = smoothDeltaTime * 0.8 + deltaTime * 0.2;
    ++frameIndex;
}
