#pragma once
#include "Core/Utilities/TimeHelpers.h"

namespace PK
{
    struct ITimer
    {
        virtual ~ITimer() = 0;

        void BeginTimerScope();
        void CaptureUnscoped();

        constexpr const double& GetScopedTimePoint() const { return scopedTimePoint; }

        virtual void EndTimerScope() = 0;
    
        private: double scopedTimePoint = TimeHelpers::GetSteadySeconds();
    };
}
