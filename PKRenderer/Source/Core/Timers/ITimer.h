#pragma once
#include "Core/Timers/TimeHelpers.h"

namespace PK
{
    struct ITimer
    {
        constexpr ITimer() = default;
        virtual ~ITimer() = 0;

        constexpr ITimer(const ITimer&) = default;
        ITimer& operator=(const ITimer&) = default;

        void BeginTimerScope();
        void CaptureUnscoped();

        constexpr const double& GetScopedTimePoint() const { return scopedTimePoint; }

        virtual void EndTimerScope() = 0;
    
        private: double scopedTimePoint = TimeHelpers::GetSteadySeconds();
    };
}
