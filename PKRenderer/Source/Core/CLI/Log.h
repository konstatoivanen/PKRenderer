#pragma once
#include <string>
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/CLI/ILogger.h"
#include "Core/CLI/LogScopeIndent.h"
#include "Core/CLI/LogScopeTimer.h"

namespace PK
{
    struct StaticLog : public NoCopy
    {
        static void SetLogger(Weak<ILogger> logger);

        static void Indent();
        static void Unindent();

        static void SetSeverityMask(LogSeverity mask);
        static void SetShowConsole(bool value);
        static void SetColor(LogColor color);

        static void LogNewLine();

        static void Log(LogSeverity severity, LogColor color, const char* format, ...);
        static void Log(LogSeverity severity, LogColor color, const std::string& format, ...);
        static void LogV(LogSeverity severity, LogColor color, const char* format, va_list args);

        static void LogRewrite(LogColor color, const char* format, ...);
        static void LogRewrite(LogColor color, const std::string& format, ...);
        static void LogRewriteV(LogColor color, const char* format, va_list args);

        static std::exception Exception(LogSeverity severity, LogColor color, const char* format, ...);
        static std::exception Exception(LogSeverity severity, LogColor color, const std::string& format, ...);
        static std::exception ExceptionV(LogSeverity severity, LogColor color, const char* format, va_list args);

    private: inline static Weak<ILogger> s_ActiveLogger;
    };
}

#define PK_LOG_NEWLINE() PK::StaticLog::LogNewLine()
#define PK_LOG_HEADER(...) PK::StaticLog::Log(PK::PK_LOG_LVL_INFO, PK::PK_LOG_COLOR_HEADER, __VA_ARGS__)
#define PK_LOG_INFO(...) PK::StaticLog::Log(PK::PK_LOG_LVL_INFO, PK::PK_LOG_COLOR_INFO, __VA_ARGS__)
#define PK_LOG_VERBOSE(...) PK::StaticLog::Log(PK::PK_LOG_LVL_VERBOSE, PK::PK_LOG_COLOR_VERBOSE, __VA_ARGS__)
#define PK_LOG_RHI(...) PK::StaticLog::Log(PK::PK_LOG_LVL_RHI, PK::PK_LOG_COLOR_RHI, __VA_ARGS__)
#define PK_LOG_WARNING(...) PK::StaticLog::Log(PK::PK_LOG_LVL_WARNING, PK::PK_LOG_COLOR_WARNING, __VA_ARGS__)
#define PK_LOG_ERROR(...) PK::StaticLog::Log(PK::PK_LOG_LVL_ERROR, PK::PK_LOG_COLOR_WARNING, __VA_ARGS__)
#define PK_LOG_OVERWRITE(...) PK::StaticLog::LogRewrite(PK::PK_LOG_COLOR_INFO, __VA_ARGS__)
#define PK_LOG_EXCEPTION(...) PK::StaticLog::Exception(PK::PK_LOG_LVL_ERROR, PK::PK_LOG_COLOR_ERROR, __VA_ARGS__)
#define PK_LOG_ADD_INDENT() PK::StaticLog::Indent()
#define PK_LOG_SUB_INDENT() PK::StaticLog::Unindent()
#define PK_THROW_ERROR(...) throw PK_LOG_EXCEPTION(__VA_ARGS__)
#define PK_THROW_ASSERT(value, ...) { if(!(value)) { PK_THROW_ERROR(__VA_ARGS__); } }
#define PK_WARNING_ASSERT(value, ...) { if(!(value)) { PK_LOG_WARNING(__VA_ARGS__); } }
#define PK_LOG_SCOPE_TIMER(name) volatile auto PK_LOG_TIMER_##name = PK::LogScopeTimer(#name)
#define PK_LOG_SCOPE_INDENT(name) auto PK_LOG_SCOPE_INDENT_##name = PK::LogScopeIndent()
