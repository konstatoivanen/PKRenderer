#include "PrecompiledHeader.h"
#include "ITimer.h"

namespace PK
{
    ITimer::~ITimer() = default;

    void ITimer::BeginTimerScope()
    {
        scopedTimePoint = TimeHelpers::GetSteadySeconds();
    }

    void ITimer::CaptureUnscoped()
    {
        EndTimerScope();
        BeginTimerScope();
    }
}
