#include "PrecompiledHeader.h"
#include "TimerFrameRunner.h"

void PK::TimerFrameRunner::EndTimerScope()
{
    unscaledDeltaTime = TimeHelpers::GetSteadySeconds() - GetScopedTimePoint();
    deltaTime = unscaledDeltaTime * timeScale;
    unscaledTime += unscaledDeltaTime;
    time += deltaTime;
    smoothDeltaTime = smoothDeltaTime * 0.8 + deltaTime * 0.2;
    ++frameIndex;
}
