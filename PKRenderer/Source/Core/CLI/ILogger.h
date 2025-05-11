#pragma once
#include <exception>

namespace PK
{
    typedef enum
    {
        PK_LOG_COLOR_BLACK = 0,
        PK_LOG_COLOR_DARK_BLUE = 1,
        PK_LOG_COLOR_DARK_GREEN = 2,
        PK_LOG_COLOR_DARK_CYAN = 3,
        PK_LOG_COLOR_DARK_RED = 4,
        PK_LOG_COLOR_DARK_MAGENTA = 5,
        PK_LOG_COLOR_DARK_YELLOW = 6,
        PK_LOG_COLOR_DARK_WHITE = 7,
        PK_LOG_COLOR_GRAY = 8,
        PK_LOG_COLOR_BLUE = 9,
        PK_LOG_COLOR_GREEN = 10,
        PK_LOG_COLOR_CYAN = 11,
        PK_LOG_COLOR_RED = 12,
        PK_LOG_COLOR_MAGENTA = 13,
        PK_LOG_COLOR_YELLOW = 14,
        PK_LOG_COLOR_WHITE = 15,
        // Severity colors
        PK_LOG_COLOR_RHI = (PK_LOG_COLOR_BLACK << 4) | PK_LOG_COLOR_GRAY,
        PK_LOG_COLOR_INFO = (PK_LOG_COLOR_BLACK << 4) | PK_LOG_COLOR_WHITE,
        PK_LOG_COLOR_VERBOSE = (PK_LOG_COLOR_BLACK << 4) | PK_LOG_COLOR_GRAY,
        PK_LOG_COLOR_HEADER = (PK_LOG_COLOR_WHITE << 4) | PK_LOG_COLOR_BLACK,
        PK_LOG_COLOR_ERROR = (PK_LOG_COLOR_DARK_RED << 4) | PK_LOG_COLOR_BLACK,
        PK_LOG_COLOR_WARNING = (PK_LOG_COLOR_YELLOW << 4) | PK_LOG_COLOR_BLACK,
        PK_LOG_COLOR_INPUT = (PK_LOG_COLOR_DARK_WHITE << 4) | PK_LOG_COLOR_CYAN
    } LogColor;

    typedef enum
    {
        PK_LOG_LVL_RHI = 1 << 0,
        PK_LOG_LVL_VERBOSE = 1 << 1,
        PK_LOG_LVL_INFO = 1 << 2,
        PK_LOG_LVL_WARNING = 1 << 3,
        PK_LOG_LVL_ERROR = 1 << 4,
        PK_LOG_LVL_ALL_FLAGS = 0xFF,
    } LogSeverity;

    constexpr static const unsigned int PK_LOG_LVL_COUNT = 5u;

    struct ILogger
    {
        virtual ~ILogger() = 0;

        virtual void Indent(LogSeverity severity) = 0;
        virtual void Unindent(LogSeverity severity) = 0;
        virtual void SetSeverityMask(LogSeverity mask) = 0;
        virtual LogSeverity GetSeverityMask() const = 0;
        virtual void SetColor(LogColor color) = 0;
        virtual void SetShowConsole(bool value) = 0;

        virtual void LogNewLine() = 0;
        virtual void LogV(LogSeverity severity, LogColor color, const char* format, va_list args) = 0;
        virtual void LogRewriteV(LogColor color, const char* format, va_list args) = 0;
        virtual std::exception ExceptionV(LogSeverity severity, LogColor color, const char* format, va_list args) = 0;
    };
}
