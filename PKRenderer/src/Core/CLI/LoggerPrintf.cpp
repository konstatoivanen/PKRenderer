#include "PrecompiledHeader.h"
#include "LoggerPrintf.h"
#include <conio.h>

namespace PK::Core::CLI
{
    void LoggerPrintf::Indent()
    {
        ++m_indentation;
    }

    void LoggerPrintf::Unindent()
    {
        if (m_indentation > 0)
        {
            --m_indentation;
        }
    }

    void LoggerPrintf::SetSeverityMask(LogSeverity mask)
    {
        m_severityMask = mask;
    }

    void LoggerPrintf::SetColor(LogColor color)
    {
#if defined(WIN32)
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#endif
    }

    void LoggerPrintf::SetShowConsole(bool value)
    {
#if defined(WIN32)
        ::ShowWindow(::GetConsoleWindow(), value ? SW_SHOW : SW_HIDE);
#endif
    }

    void LoggerPrintf::LogNewLine()
    {
        SetColor(PK_LOG_COLOR_INFO);
        printf("\n");
        m_lineClearLength = 0;
    }

    void LoggerPrintf::LogV(LogSeverity severity, LogColor color, const char* format, va_list args)
    {
        if ((severity & m_severityMask) != 0)
        {
            SetColor(color);
            LogIndent();
            auto lineLength = vprintf(format, args);
            LogLineRemainder(lineLength);
            LogNewLine();
        }
    }

    void LoggerPrintf::LogRewriteV(LogColor color, const char* format, va_list args)
    {
        SetColor(color);
        auto lineLength = vprintf(format, args);
        LogLineRemainder(lineLength);
        printf("\r");
    }

    std::exception LoggerPrintf::ExceptionV(LogSeverity severity, LogColor color, const char* format, va_list args)
    {
        if ((severity & m_severityMask) != 0)
        {
            SetColor(LogColor::PK_LOG_COLOR_BLACK);
            LogNewLine();
            SetColor(color);
            printf("--------------------PK BEGIN ERROR--------------------\n");
            vprintf(format, args);
            printf("\n--------------------PK END ERROR--------------------");
            SetColor(LogColor::PK_LOG_COLOR_BLACK);
            LogNewLine();
        }

#if defined(WIN32) && defined(PK_DEBUG)
        DebugBreak();
#endif

        _getch();
        return std::runtime_error(format);
    }

    void LoggerPrintf::LogIndent()
    {
        if (m_indentation > 0)
        {
            printf("%s", std::string(m_indentation * 4ull, ' ').c_str());
        }
    }

    void LoggerPrintf::LogLineRemainder(int32_t linelength)
    {
        auto remainder = m_lineClearLength - linelength;

        if (m_lineClearLength < linelength)
        {
            m_lineClearLength = linelength;
        }

        if (remainder > 0)
        {
            printf("%s", std::string(remainder, ' ').c_str());
        }
    }
}