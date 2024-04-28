#pragma once
#include <ctime>
#include <chrono>
#include "Core/Services/TimeFrameInfo.h"
#include "Core/Services/ServiceRegister.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "Core/ControlFlow/Sequencer.h"

namespace PK::Core::Services
{
    class Time : public IService, 
        public ControlFlow::IStepApplicationOpenFrame,
        public ControlFlow::IStepApplicationCloseFrame
    {
        public:
            Time(ControlFlow::Sequencer* sequencer, float timeScale, bool logFramerate);
    
            static const clock_t GetClockTicks();
            static const double GetClockSeconds();
    
            inline const float GetTimeScale() const { return (float)m_info.timeScale; }
            inline const float GetTime() const { return (float)m_info.time; }
            inline const float GetUnscaledTime() const { return (float)m_info.unscaledTime; }
            inline const float GetDeltaTime() const { return (float)m_info.deltaTime; }
            inline const float GetUnscaledDeltaTime() const { return (float)m_info.unscaledDeltaTime; }
            inline const float GetSmoothDeltaTime() const { return (float)m_info.smoothDeltaTime; }
            inline const uint64_t GetFrameIndex() const { return m_info.frameIndex; }
            inline const uint64_t GetFrameRate() const { return m_info.framerate; }
            inline const uint64_t GetFrameRateMin() const { return m_info.framerateMin; }
            inline const uint64_t GetFrameRateMax() const { return m_info.framerateMax; }
            inline const uint64_t GetFrameRateAvg() const { return m_info.framerateAvg; }
            inline const uint64_t GetFrameRateFixed() const { return m_info.framerateFixed; }
    
            inline void SetTimeScale(const float timeScale) { m_info.timeScale = (double)timeScale; }
            inline void SetLogFramerate(const bool value) { m_logFramerate = value; }
            
            void Reset();
            void LogFrameRate();
    
            virtual void OnApplicationOpenFrame() final;
            virtual void OnApplicationCloseFrame() final;
    
        private:
            ControlFlow::Sequencer* m_sequencer = nullptr;
            std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> m_frameStart;
            TimeFrameInfo m_info{};
            uint64_t m_framerateMinRaw = 0u;
            uint64_t m_framerateMaxRaw = 0u;
            uint64_t m_frameIndexFixed = 0;
            uint64_t m_second = 0;
            bool m_logFramerate = true;
    };
}