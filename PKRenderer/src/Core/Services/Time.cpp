#include "PrecompiledHeader.h"
#include "Core/Services/Log.h"
#include "Core/Services/Time.h"
#include <ctime>
#include <cstdlib>
#include <Core/UpdateStep.h>
#include <ECS/Contextual/Tokens/TimeToken.h>

namespace PK::Core::Services
{
    const clock_t Time::GetClockTicks()
    {
        return clock();
    }
    
    const double Time::GetClockSeconds()
    {
        return clock() / (double)CLOCKS_PER_SEC;
    }
    
    Time::Time(Sequencer* sequencer, float timeScale) : m_sequencer(sequencer), m_timeScale(timeScale)
    {
    }
    
  
    void Time::Reset()
    {
        m_time = 0;
        m_unscaledTime = 0;
    
        m_frameIndex = 0;
        m_frameIndexFixed = 0;
        m_framerate = 0;
        m_framerateMin = (uint64_t)-1;
        m_framerateMax = 0;
        m_framerateAvg = 0;
        m_framerateFixed = 0;
        m_second = 0;
    }
    
    void Time::LogFrameRate()
    {
        PK_LOG_OVERWRITE("FPS: %4.1i, FIXED: %i, MIN: %i, MAX: %i, AVG: %i MS: %4.2f", m_framerate, m_framerateFixed, m_framerateMin, m_framerateMax, m_framerateAvg, m_unscaledDeltaTimeFixed * 1000.0f);
    }
    
    void Time::Step(int condition)
    {
        auto step = (PK::Core::UpdateStep)condition;

        switch (step)
        {
            case PK::Core::UpdateStep::OpenFrame: 
            {
                m_frameStart = std::chrono::steady_clock::now(); 

                PK::ECS::Tokens::TimeToken token;
                token.timeScale = m_timeScale;
                token.time = m_time;
                token.unscaledTime = m_unscaledTime;
                token.deltaTime = m_deltaTime;
                token.unscaledDeltaTime = m_unscaledDeltaTime;
                token.smoothDeltaTime = m_smoothDeltaTime;
                token.unscaledDeltaTimeFixed = m_unscaledDeltaTimeFixed;

                m_sequencer->Next<PK::ECS::Tokens::TimeToken>(this, &token, 0);

                if (token.logFrameRate)
                {
                    LogFrameRate();
                }
            }
            break;
            case PK::Core::UpdateStep::CloseFrame:
            {
                std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> frameEnd = std::chrono::steady_clock::now();
                m_unscaledDeltaTime = (frameEnd - m_frameStart).count();
                m_deltaTime = m_unscaledDeltaTime * m_timeScale;

                m_unscaledTime += m_unscaledDeltaTime;
                m_time += m_deltaTime;

                m_smoothDeltaTime = m_smoothDeltaTime + 0.5 * (m_deltaTime - m_smoothDeltaTime);

                ++m_frameIndex;

                if (m_unscaledDeltaTime > 0)
                {
                    m_framerate = (uint64_t)(1.0 / m_unscaledDeltaTime);
                }

                if ((uint64_t)m_unscaledTime != m_second)
                {
                    m_framerateFixed = m_frameIndex - m_frameIndexFixed;
                    m_frameIndexFixed = m_frameIndex;
                    m_unscaledDeltaTimeFixed = 1.0 / m_framerateFixed;
                    m_second = (uint64_t)m_unscaledTime;
                    m_framerateMin = (uint64_t)-1;
                    m_framerateMax = 0;
                }

                if (m_framerate < m_framerateMin)
                {
                    m_framerateMin = m_framerate;
                }

                if (m_framerate > m_framerateMax)
                {
                    m_framerateMax = m_framerate;
                }

                m_framerateAvg = (uint64_t)(m_frameIndex / m_unscaledTime);
            }
            break;
        }
    }
}