#include "PrecompiledHeader.h"
#include "Log.h"

namespace PK::Core::Services::Debug
{
    // Hack: global as logger gets released before final log...
    // @TODO fix this
    static int32_t s_Indentation = 0u;

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

    void Logger::InsertIndentation()
    {
        if (s_Indentation > 0)
        {
            printf(std::string(s_Indentation * 4, ' ').c_str());
        }
    }

    void Logger::InsertNewLine()
    {
        SetConsoleColor((int)ConsoleColor::LOG_INFO);
        printf("\n");
        m_lineClearLength = 0;
    }

    void Logger::AddIndent()
    {
        ++s_Indentation;
    }

    void Logger::SubIndent()
    {
        if (s_Indentation > 0)
        {
            --s_Indentation;
        }
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

    IndentScope::IndentScope()
    {
        PK_LOG_ADD_INDENT();
    }

    IndentScope::~IndentScope()
    {
        PK_LOG_SUB_INDENT();
    }
}