#include "PrecompiledHeader.h"
#include <ctime>
#include <cstdlib>
#include "Core/CLI/CVariableRegister.h"
#include "Core/CLI/Log.h"
#include "Core/ControlFlow/Sequencer.h"
#include "App/FrameContext.h"
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

    EngineTime::EngineTime(Sequencer* sequencer, float timeScale) : m_sequencer(sequencer)
    {
        m_runner.timeScale = timeScale;
        CVariableRegister::Create<CVariableFuncSimple>("Time.Reset", [this]() { Reset(); PK_LOG_INFO("Time.Reset"); });
    }


    void EngineTime::Reset()
    {
        auto timeScale = m_runner.timeScale;
        m_runner = TimerFrameRunner();
        m_framerate = TimerFramerate();
        m_runner.timeScale = timeScale;
    }

    void EngineTime::OnStepFrameInitialize(FrameContext* ctx)
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
        ctx->time = frameTimeInfo;
        m_sequencer->Next(this, &frameTimeInfo);

        TimeFramerateInfo framerateInfo{};
        framerateInfo.framerate = m_framerate.framerate;
        framerateInfo.framerateMin = m_framerateFixed.framerateMin;
        framerateInfo.framerateMax = m_framerateFixed.framerateMax;
        framerateInfo.framerateAvg = m_framerateFixed.framerateAvg;
        framerateInfo.frameMs = m_framerate.deltaTime * 1000.0f;
        ctx->framerate = framerateInfo;
        m_sequencer->Next(this, &framerateInfo);
    }

    void EngineTime::OnStepFrameFinalize(FrameContext* ctx)
    {
        m_runner.EndTimerScope();
        m_framerate.CaptureUnscoped();

        if (m_framerate.elapsed > 1.0)
        {
            m_framerateFixed = m_framerate;
            m_framerate = TimerFramerate();
            m_framerate.deltaTime = m_framerateFixed.deltaTime;
            m_framerate.framerate = m_framerateFixed.framerate;
        }

        // apply potential changes in time scale.
        m_runner.timeScale = ctx->time.timeScale;
    }
}
