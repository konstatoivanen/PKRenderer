#pragma once
#include <ctime>
#include <chrono>
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/TimeFrameInfo.h"
#include "Core/ControlFlow/IStepApplication.h"

namespace PK
{
    class Sequencer;

    class Time :
        public IStepApplicationOpenFrame,
        public IStepApplicationCloseFrame
    {
    public:
        Time(Sequencer* sequencer, float timeScale, bool logFramerate);

        static clock_t GetClockTicks();
        static double GetClockSeconds();

        inline float GetTimeScale() const { return (float)m_info.timeScale; }
        inline float GetTime() const { return (float)m_info.time; }
        inline float GetUnscaledTime() const { return (float)m_info.unscaledTime; }
        inline float GetDeltaTime() const { return (float)m_info.deltaTime; }
        inline float GetUnscaledDeltaTime() const { return (float)m_info.unscaledDeltaTime; }
        inline float GetSmoothDeltaTime() const { return (float)m_info.smoothDeltaTime; }
        inline uint64_t GetFrameIndex() const { return m_info.frameIndex; }
        inline uint64_t GetFrameRate() const { return m_info.framerate; }
        inline uint64_t GetFrameRateMin() const { return m_info.framerateMin; }
        inline uint64_t GetFrameRateMax() const { return m_info.framerateMax; }
        inline uint64_t GetFrameRateAvg() const { return m_info.framerateAvg; }
        inline uint64_t GetFrameRateFixed() const { return m_info.framerateFixed; }

        inline void SetTimeScale(const float timeScale) { m_info.timeScale = (double)timeScale; }
        inline void SetLogFramerate(const bool value) { m_logFramerate = value; }

        void Reset();
        void LogFrameRate();

        virtual void OnApplicationOpenFrame() final;
        virtual void OnApplicationCloseFrame() final;

    private:
        Sequencer* m_sequencer = nullptr;
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> m_frameStart;
        TimeFrameInfo m_info{};
        uint64_t m_framerateMinRaw = 0u;
        uint64_t m_framerateMaxRaw = 0u;
        uint64_t m_frameIndexFixed = 0;
        uint64_t m_second = 0;
        bool m_logFramerate = true;
    };
}