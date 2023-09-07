#include "PrecompiledHeader.h"
#include "Log.h"

namespace PK::Core::Services::Debug
{
    void Logger::ClearLineRemainder(int32_t length)
    {
        auto l = m_lineClearLength - length;

        if (m_lineClearLength < length)
        {
            m_lineClearLength = length;
        }

        if (l <= 0)
        {
            return;
        }

        printf(std::string(l, ' ').c_str());
    }

    void Logger::InsertNewLine()
    {
        SetConsoleColor((int)ConsoleColor::LOG_PARAMETER);
        printf("\n");
        m_lineClearLength = 0;
    }

    ScopeTimer::ScopeTimer(const char* name) : 
        start(std::chrono::steady_clock::now()),
        name(name)
    {
    }

    ScopeTimer::~ScopeTimer()
    {
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> end = std::chrono::steady_clock::now();
        auto delta = (end - start) * 1000.0;
        PK_LOG_INFO("ScopeTimer: %s, %4.4f ms", name, delta);
    }
}