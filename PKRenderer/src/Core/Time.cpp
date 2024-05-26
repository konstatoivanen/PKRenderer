#include "PrecompiledHeader.h"
#include <ctime>
#include <cstdlib>
#include "Core/CLI/CVariableRegister.h"
#include "Core/CLI/Log.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Time.h"

namespace PK
{
    const clock_t Time::GetClockTicks()
    {
        return clock();
    }

    const double Time::GetClockSeconds()
    {
        return clock() / (double)CLOCKS_PER_SEC;
    }

    Time::Time(Sequencer* sequencer, float timeScale, bool logFramerate) :
        m_sequencer(sequencer),
        m_logFramerate(logFramerate)
    {
        m_info.timeScale = timeScale;
        CVariableRegister::Create<CVariableFunc>("Time.Reset", [this](const char** args, uint32_t count) { Reset(); PK_LOG_INFO("Time.Reset"); });
        CVariableRegister::Create<bool*>("Time.LogFramerate", &m_logFramerate, "0 = 0ff, 1 = On", 1u, 1u);
    }


    void Time::Reset()
    {
        auto timeScale = m_info.timeScale;
        m_info = {};
        m_info.timeScale = timeScale;
        m_second = 0;
    }

    void Time::LogFrameRate()
    {
        PK_LOG_OVERWRITE("FPS: %4.1i, FIXED: %i, MIN: %i, MAX: %i, AVG: %i MS: %4.2f",
            m_info.framerate,
            m_info.framerateFixed,
            m_info.framerateMin,
            m_info.framerateMax,
            m_info.framerateAvg,
            m_info.unscaledDeltaTimeFixed * 1000.0f);
    }

    void Time::OnApplicationOpenFrame()
    {
        m_frameStart = std::chrono::steady_clock::now();
        auto frameTimeInfo = m_info;
        m_sequencer->Next(this, &frameTimeInfo);
    }

    void Time::OnApplicationCloseFrame()
    {
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> frameEnd = std::chrono::steady_clock::now();
        m_info.unscaledDeltaTime = (frameEnd - m_frameStart).count();
        m_info.deltaTime = m_info.unscaledDeltaTime * m_info.timeScale;

        m_info.unscaledTime += m_info.unscaledDeltaTime;
        m_info.time += m_info.deltaTime;
        m_info.smoothDeltaTime = m_info.smoothDeltaTime + 0.5 * (m_info.deltaTime - m_info.smoothDeltaTime);

        ++m_info.frameIndex;

        if (m_info.unscaledDeltaTime > 0)
        {
            m_info.framerate = (uint64_t)(1.0 / m_info.unscaledDeltaTime);
        }

        if (m_info.framerate < m_framerateMinRaw)
        {
            m_framerateMinRaw = m_info.framerate;
        }

        if (m_info.framerate > m_framerateMaxRaw)
        {
            m_framerateMaxRaw = m_info.framerate;
        }

        m_info.framerateAvg = (uint64_t)(m_info.frameIndex / m_info.unscaledTime);

        if ((uint64_t)m_info.unscaledTime != m_second)
        {
            m_info.framerateFixed = m_info.frameIndex - m_frameIndexFixed;
            m_frameIndexFixed = m_info.frameIndex;
            m_info.unscaledDeltaTimeFixed = 1.0 / m_info.framerateFixed;
            m_info.framerateMin = m_framerateMinRaw;
            m_info.framerateMax = m_framerateMaxRaw;
            m_second = (uint64_t)m_info.unscaledTime;
            m_framerateMinRaw = (uint64_t)-1;
            m_framerateMaxRaw = 0ull;
        }

        if (m_logFramerate)
        {
            LogFrameRate();
        }
    }
}