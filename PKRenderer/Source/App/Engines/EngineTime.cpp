#include "PrecompiledHeader.h"
#include <ctime>
#include <cstdlib>
#include "Core/CLI/CVariableRegister.h"
#include "Core/CLI/Log.h"
#include "Core/ControlFlow/Sequencer.h"
#include "EngineTime.h"

namespace PK::App
{
    clock_t EngineTime::GetClockTicks()
    {
        return clock();
    }

    double EngineTime::GetClockSeconds()
    {
        return clock() / (double)CLOCKS_PER_SEC;
    }

    EngineTime::EngineTime(Sequencer* sequencer, float timeScale, bool logFramerate) :
        m_sequencer(sequencer),
        m_logFramerate(logFramerate)
    {
        m_runner.timeScale = timeScale;
        CVariableRegister::Create<CVariableFuncSimple>("Time.Reset", [this]() { Reset(); PK_LOG_INFO("Time.Reset"); });
        CVariableRegister::Create<bool*>("Time.LogFramerate", &m_logFramerate, "0 = 0ff, 1 = On", 1u);
    }


    void EngineTime::Reset()
    {
        auto timeScale = m_runner.timeScale;
        m_runner = TimerFrameRunner();
        m_framerate = TimerFramerate();
        m_runner.timeScale = timeScale;
    }

    void EngineTime::LogFrameRate()
    {
        PK_LOG_OVERWRITE("FPS: %4.1i, FIXED: %i, MIN: %i, MAX: %i, AVG: %i MS: %4.2f",
            m_framerate.framerate,
            m_framerateFixed.frameCount,
            m_framerateFixed.framerateMin,
            m_framerateFixed.framerateMax,
            m_framerateFixed.framerateAvg,
            (1.0 / m_framerateFixed.framerateAvg) * 1000.0f);
    }

    void EngineTime::OnApplicationOpenFrame()
    {
        m_runner.BeginTimerScope();

        TimeFrameInfo frameTimeInfo{};
        frameTimeInfo.frameIndex = m_runner.frameIndex;
        frameTimeInfo.timeScale = m_runner.timeScale;
        frameTimeInfo.time = m_runner.time;
        frameTimeInfo.unscaledTime = m_runner.unscaledTime;
        frameTimeInfo.deltaTime = m_runner.deltaTime;
        frameTimeInfo.unscaledDeltaTime = m_runner.unscaledDeltaTime;
        frameTimeInfo.smoothDeltaTime = m_runner.smoothDeltaTime;
        m_sequencer->Next(this, &frameTimeInfo);
    }

    void EngineTime::OnApplicationCloseFrame()
    {
        m_runner.EndTimerScope();
        m_framerate.CaptureUnscoped();

        if (m_framerate.elapsed > 1.0)
        {
            m_framerateFixed = m_framerate;
            m_framerate = TimerFramerate();
        }

        if (m_logFramerate)
        {
            LogFrameRate();
        }
    }
}