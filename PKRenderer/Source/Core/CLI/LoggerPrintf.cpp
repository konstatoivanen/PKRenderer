#include "PrecompiledHeader.h"
#include <conio.h>
#include "Core/Utilities/FileIOBinary.h"
#include "LoggerPrintf.h"

namespace PK
{
    void LoggerPrintf::SetCrashLogPath(const char* value)
    {
        m_crashLogPath = FixedString256(value);
    }

    void LoggerPrintf::SetSeverityMask(LogSeverity mask)
    {
        m_severityMask = mask;
    }

    LogSeverity LoggerPrintf::GetSeverityMask() const
    {
        return (LogSeverity)m_severityMask;
    }

    void LoggerPrintf::SetColor(LogColor color)
    {
        Platform::SetConsoleColor(color);
    }

    void LoggerPrintf::SetShowConsole(bool value)
    {
        Platform::SetConsoleVisible(value);
    }

    void LoggerPrintf::Indent(LogSeverity severity)
    {
        for (auto i = 0u; i < PK_LOG_LVL_COUNT; ++i)
        {
            if ((severity & (1u << i)) != 0 && m_indentation[i] < (int32_t)MAX_INDENT)
            {
                ++m_indentation[i];
            }
        }
    }

    void LoggerPrintf::Outdent(LogSeverity severity)
    {
        for (auto i = 0u; i < PK_LOG_LVL_COUNT; ++i)
        {
            if ((severity & (1u << i)) != 0 && m_indentation[i] > 0)
            {
                --m_indentation[i];
            }
        }
    }

    void LoggerPrintf::NewLine()
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
            NewLine();
        }
    }

    std::exception LoggerPrintf::ExceptionV(LogSeverity severity, LogColor color, const char* format, va_list args)
    {
        if (m_crashLogPath.Length() > 0)
        {
            auto length = vprintf(format, args);
            std::string output;
            output.resize(length);
            _vsnprintf(output.data(), output.size(), format, args);
            FileIO::WriteBinary(m_crashLogPath.c_str(), true, output.data(), output.size());
        }

        if ((severity & m_severityMask) != 0)
        {
            SetColor(LogColor::PK_LOG_COLOR_BLACK);
            NewLine();
            SetColor(color);
            printf("--------------------PK BEGIN ERROR--------------------\n");
            vprintf(format, args);
            printf("\n--------------------PK END ERROR--------------------");
            SetColor(LogColor::PK_LOG_COLOR_BLACK);
            NewLine();
        }

        PK_PLATFORM_DEBUG_BREAK;
        
        // Forces flush
        _getch();
        
        return std::runtime_error(format);
    }

    void LoggerPrintf::LogIndent()
    {
        auto indendation = 0;

        for (auto i = 0u; i < PK_LOG_LVL_COUNT; ++i)
        {
            if ((m_severityMask & (1 << i)) != 0)
            {
                indendation += m_indentation[i];
            }
        }

        if (indendation > 0)
        {
            char spaces[MAX_INDENT * 4u + 1u];
            memset(spaces, ' ', sizeof(spaces));
            spaces[indendation * 4u] = 0;
            printf("%s", spaces);
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
