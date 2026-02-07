#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/Parse.h"
#include "Core/CLI/ILogger.h"
#include "Core/CLI/LogScopeIndent.h"
#include "Core/CLI/LogScopeTimer.h"

namespace PK
{
    struct StaticLog : public NoCopy
    {
        static void SetLogger(Weak<ILogger> logger);

        static void SetCrashLogPath(const char* value);
        static void SetSeverityMask(LogSeverity mask);
        static void SetSeverityMaskFlag(LogSeverity flag, bool value);
        static LogSeverity GetSeverityMask();
        static void SetShowConsole(bool value);
        static void SetColor(LogColor color);

        static void Indent(LogSeverity severity = PK_LOG_LVL_ALL_FLAGS);
        static void Outdent(LogSeverity severity = PK_LOG_LVL_ALL_FLAGS);
        static void NewLine();
        static void Log(LogSeverity severity, LogColor color, const char* format, ...);
        static void Log(LogSeverity severity, LogColor color, const std::string& format, ...);
        static void LogV(LogSeverity severity, LogColor color, const char* format, va_list args);
        static std::exception Exception(LogSeverity severity, LogColor color, const char* format, ...);
        static std::exception Exception(LogSeverity severity, LogColor color, const std::string& format, ...);
        static std::exception ExceptionV(LogSeverity severity, LogColor color, const char* format, va_list args);

        private: 
            inline static Weak<ILogger> s_ActiveLogger;
    };
}

#define PK_LOG_CONCAT_INNER(a, b) a##b
#define PK_LOG_CONCAT(a, b) PK_LOG_CONCAT_INNER(a,b)
#define PK_LOG_UNIQUE_NAME(base) PK_LOG_CONCAT(base, __COUNTER__)
#define PK_LOG_SHORT_FUNCTION_NAME_PARAMS() static_cast<int>(Parse::GetShortFunctionNameLength(__PRETTY_FUNCTION__)), Parse::GetShortFunctionNameData(__PRETTY_FUNCTION__)

#if defined(PK_NO_LOGS)
    #define PK_LOG_NEWLINE()
    #define PK_LOG_HEADER(...)
    #define PK_LOG_INFO(...)
    #define PK_LOG_VERBOSE(...)
    #define PK_LOG_RHI(...)
    #define PK_LOG_WARNING(...)
    #define PK_LOG_ERROR(...)
    #define PK_LOG_INDENT(severity)
#else
    #define PK_LOG_NEWLINE() PK::StaticLog::NewLine()
    #define PK_LOG_HEADER(...) PK::StaticLog::Log(PK::PK_LOG_LVL_INFO, PK::PK_LOG_COLOR_HEADER, __VA_ARGS__)
    #define PK_LOG_INFO(...) PK::StaticLog::Log(PK::PK_LOG_LVL_INFO, PK::PK_LOG_COLOR_INFO, __VA_ARGS__)
    #define PK_LOG_VERBOSE(...) PK::StaticLog::Log(PK::PK_LOG_LVL_VERBOSE, PK::PK_LOG_COLOR_VERBOSE, __VA_ARGS__)
    #define PK_LOG_RHI(...) PK::StaticLog::Log(PK::PK_LOG_LVL_RHI, PK::PK_LOG_COLOR_RHI, __VA_ARGS__)
    #define PK_LOG_WARNING(...) PK::StaticLog::Log(PK::PK_LOG_LVL_WARNING, PK::PK_LOG_COLOR_WARNING, __VA_ARGS__)
    #define PK_LOG_ERROR(...) PK::StaticLog::Log(PK::PK_LOG_LVL_ERROR, PK::PK_LOG_COLOR_WARNING, __VA_ARGS__)
    #define PK_LOG_INDENT(severity) auto PK_LOG_UNIQUE_NAME(pk_log_scope_indent_) = PK::LogScopeIndent(severity)
#endif

#define PK_LOG_EXCEPTION(...) PK::StaticLog::Exception(PK::PK_LOG_LVL_ERROR, PK::PK_LOG_COLOR_ERROR, __VA_ARGS__)

#define PK_THROW_ERROR(...) throw PK_LOG_EXCEPTION(__VA_ARGS__)
#define PK_THROW_ASSERT(value, ...) { if(!(value)) { PK_THROW_ERROR(__VA_ARGS__); } }
#define PK_WARNING_ASSERT(value, ...) { if(!(value)) { PK_LOG_WARNING(__VA_ARGS__); } }

#if PK_DEBUG
#define PK_DEBUG_THROW_ASSERT(value, ...) { if(!(value)) { PK_THROW_ERROR(__VA_ARGS__); } }
#define PK_DEBUG_WARNING_ASSERT(value, ...) { if(!(value)) { PK_LOG_WARNING(__VA_ARGS__); } }
#else
#define PK_DEBUG_THROW_ASSERT(value, ...)
#define PK_DEBUG_WARNING_ASSERT(value, ...)
#endif

#define PK_LOG_TIMER(name) volatile auto PK_LOG_UNIQUE_NAME(pk_log_scope_timer_) = PK::LogScopeTimer(name)
#define PK_LOG_TIMER_FUNC() volatile auto PK_LOG_UNIQUE_NAME(pk_log_scope_timer_) = PK::LogScopeTimer(PK_LOG_SHORT_FUNCTION_NAME_PARAMS())

#define PK_LOG_HEADER_SCOPE(...) PK_LOG_HEADER(__VA_ARGS__); PK_LOG_INDENT(PK::PK_LOG_LVL_INFO)
#define PK_LOG_INFO_SCOPE(...) PK_LOG_INFO(__VA_ARGS__); PK_LOG_INDENT(PK::PK_LOG_LVL_INFO)
#define PK_LOG_VERBOSE_SCOPE(...) PK_LOG_VERBOSE(__VA_ARGS__); PK_LOG_INDENT(PK::PK_LOG_LVL_VERBOSE)
#define PK_LOG_RHI_SCOPE(...) PK_LOG_RHI(__VA_ARGS__); PK_LOG_INDENT(PK::PK_LOG_LVL_RHI)

#define PK_LOG_HEADER_FUNC(format, ...) PK_LOG_HEADER("%.*s: " format, PK_LOG_SHORT_FUNCTION_NAME_PARAMS(), __VA_ARGS__); PK_LOG_INDENT(PK::PK_LOG_LVL_INFO)
#define PK_LOG_INFO_FUNC(format, ...) PK_LOG_INFO("%.*s: " format, PK_LOG_SHORT_FUNCTION_NAME_PARAMS(), __VA_ARGS__); PK_LOG_INDENT(PK::PK_LOG_LVL_INFO)
#define PK_LOG_VERBOSE_FUNC(format, ...) PK_LOG_VERBOSE("%.*s: " format, PK_LOG_SHORT_FUNCTION_NAME_PARAMS(), __VA_ARGS__); PK_LOG_INDENT(PK::PK_LOG_LVL_VERBOSE)
#define PK_LOG_RHI_FUNC(format, ...) PK_LOG_RHI("%.*s: " format, PK_LOG_SHORT_FUNCTION_NAME_PARAMS(), __VA_ARGS__); PK_LOG_INDENT(PK::PK_LOG_LVL_RHI)
