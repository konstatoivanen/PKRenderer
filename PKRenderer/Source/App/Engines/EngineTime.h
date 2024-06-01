#pragma once
#include <ctime>
#include <chrono>
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Timers/TimeFrameInfo.h"
#include "Core/Timers/TimerFramerate.h"
#include "Core/Timers/TimerFrameRunner.h"
#include "Core/ControlFlow/IStepApplication.h"

namespace PK
{
    class Sequencer;
}

namespace PK::App
{
    class EngineTime :
        public IStepApplicationOpenFrame,
        public IStepApplicationCloseFrame
    {
    public:
        EngineTime(Sequencer* sequencer, float timeScale, bool logFramerate);

        static clock_t GetClockTicks();
        static double GetClockSeconds();

        inline float GetTimeScale() const { return (float)m_runner.timeScale; }
        inline float GetTime() const { return (float)m_runner.time; }
        inline float GetUnscaledTime() const { return (float)m_runner.unscaledTime; }
        inline float GetDeltaTime() const { return (float)m_runner.deltaTime; }
        inline float GetUnscaledDeltaTime() const { return (float)m_runner.unscaledDeltaTime; }
        inline float GetSmoothDeltaTime() const { return (float)m_runner.smoothDeltaTime; }
        inline uint64_t GetFrameIndex() const { return m_runner.frameIndex; }
        inline uint64_t GetFrameRate() const { return m_framerate.framerate; }
        inline uint64_t GetFrameRateMin() const { return m_framerateFixed.framerateMin; }
        inline uint64_t GetFrameRateMax() const { return m_framerateFixed.framerateMax; }
        inline uint64_t GetFrameRateAvg() const { return m_framerateFixed.framerateAvg; }
        inline uint64_t GetFrameRateFixed() const { return m_framerateFixed.frameCount; }

        inline void SetTimeScale(const float timeScale) { m_runner.timeScale = (double)timeScale; }
        inline void SetLogFramerate(const bool value) { m_logFramerate = value; }

        void Reset();
        void LogFrameRate();

        virtual void OnApplicationOpenFrame() final;
        virtual void OnApplicationCloseFrame() final;

    private:
        Sequencer* m_sequencer = nullptr;
        TimerFramerate m_framerate{};
        TimerFramerate m_framerateFixed{};
        TimerFrameRunner m_runner{};
        bool m_logFramerate = true;
    };
}