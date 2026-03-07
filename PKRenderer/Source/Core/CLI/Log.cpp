#include "PrecompiledHeader.h"
#include "Log.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvarargs"
#endif

namespace PK
{
    #if PK_LOG_USE_SATIC_SEVERITY_MASK
        #define PK_LOG_ASSIGN_STATIC_SEVERITY_MASK(var, logger) var = logger.IsAlive() ? logger.Lock()->GetSeverityMask() : LogSeverity::PK_LOG_LVL_NONE
        #define PK_LOG_CHECK_STATIC_SEVERITY_MASK(var, mask) ((var & mask) != 0)
    #else
        #define PK_LOG_ASSIGN_STATIC_SEVERITY_MASK(var, logger) 
        #define PK_LOG_CHECK_STATIC_SEVERITY_MASK(var, mask) true
    #endif

    #define PK_FORWAD_VARGS_FUNC(fmt, args, Func) \
        va_list args; \
        va_start(args, fmt); \
        Func \
        va_end(args); \

    ILogger::~ILogger() = default;


    void StaticLog::SetLogger(Weak<ILogger> logger)
    {
        s_ActiveLogger = logger;
        PK_LOG_ASSIGN_STATIC_SEVERITY_MASK(s_SeverityMask, logger);
    }

    void StaticLog::SetCrashLogPath(const char* value)
    {
        if (s_ActiveLogger.IsAlive())
        {
            s_ActiveLogger.Lock()->SetCrashLogPath(value);
        }
    }

    void StaticLog::SetSeverityMask(LogSeverity mask)
    {
        if (s_ActiveLogger.IsAlive())
        {
            s_ActiveLogger.Lock()->SetSeverityMask(mask);
            PK_LOG_ASSIGN_STATIC_SEVERITY_MASK(s_SeverityMask, s_ActiveLogger);
        }
    }

    void StaticLog::SetSeverityMaskFlag(LogSeverity flag, bool value)
    {
        if (s_ActiveLogger.IsAlive())
        {
            auto logger = s_ActiveLogger.Lock();
            uint32_t filter = logger->GetSeverityMask();
            logger->SetSeverityMask((LogSeverity)(value ? (filter | flag) : (filter & ~flag)));
            PK_LOG_ASSIGN_STATIC_SEVERITY_MASK(s_SeverityMask, s_ActiveLogger);
        }
    }

    LogSeverity StaticLog::GetSeverityMask()
    {
        if (s_ActiveLogger.IsAlive())
        {
            return s_ActiveLogger.Lock()->GetSeverityMask();
        }

        return (LogSeverity)0u;
    }

    void StaticLog::SetShowConsole(bool value)
    {
        if (s_ActiveLogger.IsAlive())
        {
            s_ActiveLogger.Lock()->SetShowConsole(value);
        }
    }

    void StaticLog::SetColor(LogColor color)
    {
        if (s_ActiveLogger.IsAlive())
        {
            s_ActiveLogger.Lock()->SetColor(color);
        }
    }

    void StaticLog::Indent(LogSeverity severity)
    {
        if (PK_LOG_CHECK_STATIC_SEVERITY_MASK(s_SeverityMask, severity) && s_ActiveLogger.IsAlive())
        {
            s_ActiveLogger.Lock()->Indent(severity);
        }
    }

    void StaticLog::Outdent(LogSeverity severity)
    {
        if (PK_LOG_CHECK_STATIC_SEVERITY_MASK(s_SeverityMask, severity) && s_ActiveLogger.IsAlive())
        {
            s_ActiveLogger.Lock()->Outdent(severity);
        }
    }

    void StaticLog::NewLine()
    {
        if (s_ActiveLogger.IsAlive())
        {
            s_ActiveLogger.Lock()->NewLine();
        }
    }

    void StaticLog::Log(LogSeverity severity, LogColor color, const char* format, ...)
    {
        if (PK_LOG_CHECK_STATIC_SEVERITY_MASK(s_SeverityMask, severity))
        {
            PK_FORWAD_VARGS_FUNC(format, log_args, LogV(severity, color, format, log_args);)
        }
    }

    void StaticLog::LogV(LogSeverity severity, LogColor color, const char* format, va_list args)
    {
        if (PK_LOG_CHECK_STATIC_SEVERITY_MASK(s_SeverityMask, severity) && s_ActiveLogger.IsAlive())
        {
            s_ActiveLogger.Lock()->LogV(severity, color, format, args);
        }
    }

    std::exception StaticLog::Exception(LogSeverity severity, LogColor color, const char* format, ...)
    {
        PK_FORWAD_VARGS_FUNC(format, log_args, auto exception = ExceptionV(severity, color, format, log_args);)
        return exception;
    }

    std::exception StaticLog::ExceptionV(LogSeverity severity, LogColor color, const char* format, va_list args)
    {
        if (PK_LOG_CHECK_STATIC_SEVERITY_MASK(s_SeverityMask, severity) && s_ActiveLogger.IsAlive())
        {
            return s_ActiveLogger.Lock()->ExceptionV(severity, color, format, args);
        }

        return std::exception(format);
    }

    #undef PK_FORWAD_VARGS_FUNC
    #undef PK_LOG_ASSIGN_STATIC_SEVERITY_MASK
    #undef PK_LOG_CHECK_STATIC_SEVERITY_MASK

}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
