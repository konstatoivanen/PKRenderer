#pragma once
#include "Core/Timers/TimeHelpers.h"

namespace PK
{
    struct ITimer
    {
        virtual ~ITimer() = 0;

        void BeginTimerScope();
        void CaptureUnscoped();

        constexpr const TimePointSeconds& GetScopedTimePoint() const { return scopedTimePoint; }

        virtual void EndTimerScope() = 0;
    
        private: TimePointSeconds scopedTimePoint = TimeHelpers::GetTimePointSecondsNow();
    };
}
