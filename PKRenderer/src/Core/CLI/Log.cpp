#include "PrecompiledHeader.h"
#include "Log.h"

namespace PK::Core::CLI
{
    using namespace PK::Utilities;

    #define PK_FORWAD_VARGS_FUNC(fmt, args, Func)   \
        va_list args;                               \
        va_start(args, fmt);                        \
        Func                                        \
        va_end(args);                               \

    void StaticLog::SetLogger(Weak<ILogger> logger)
    {
        s_ActiveLogger = logger;
    }

    void StaticLog::Indent()
    {
        if (!s_ActiveLogger.expired())
        {
            s_ActiveLogger.lock()->Indent();
        }
    }

    void StaticLog::Unindent()
    {
        if (!s_ActiveLogger.expired())
        {
            s_ActiveLogger.lock()->Unindent();
        }
    }

    void StaticLog::SetSeverityMask(LogSeverity mask)
    {
        if (!s_ActiveLogger.expired())
        {
            s_ActiveLogger.lock()->SetSeverityMask(mask);
        }
    }

    void StaticLog::SetColor(LogColor color)
    {
        if (!s_ActiveLogger.expired())
        {
            s_ActiveLogger.lock()->SetColor(color);
        }
    }

    void StaticLog::LogNewLine()
    {
        if (!s_ActiveLogger.expired())
        {
            s_ActiveLogger.lock()->LogNewLine();
        }
    }

    void StaticLog::Log(LogSeverity severity, LogColor color, const char* format, ...)
    {
        PK_FORWAD_VARGS_FUNC(format, log_args, LogV(severity, color, format, log_args);)
    }

    void StaticLog::Log(LogSeverity severity, LogColor color, const std::string& format, ...)
    {
        const char* cformat = format.c_str();
        PK_FORWAD_VARGS_FUNC(cformat, log_args, LogV(severity, color, cformat, log_args);)
    }

    void StaticLog::LogV(LogSeverity severity, LogColor color, const char* format, va_list args)
    {
        if (!s_ActiveLogger.expired())
        {
            s_ActiveLogger.lock()->LogV(severity, color, format, args);
        }
    }

    void StaticLog::LogRewrite(LogColor color, const char* format, ...)
    {
        PK_FORWAD_VARGS_FUNC(format, log_args, LogRewriteV(color, format, log_args);)
    }

    void StaticLog::LogRewrite(LogColor color, const std::string& format, ...)
    {
        const char* cformat = format.c_str();
        PK_FORWAD_VARGS_FUNC(cformat, log_args, LogRewriteV(color, cformat, log_args);)
    }

    void StaticLog::LogRewriteV(LogColor color, const char* format, va_list args)
    {
        if (!s_ActiveLogger.expired())
        {
            s_ActiveLogger.lock()->LogRewriteV(color, format, args);
        }
    }

    std::exception StaticLog::Exception(LogSeverity severity, LogColor color, const char* format, ...)
    {
        PK_FORWAD_VARGS_FUNC(format, log_args, auto exception = ExceptionV(severity, color, format, log_args);)
        return exception;
    }

    std::exception StaticLog::Exception(LogSeverity severity, LogColor color, const std::string& format, ...)
    {
        const char* cformat = format.c_str();
        PK_FORWAD_VARGS_FUNC(cformat, log_args, auto exception = ExceptionV(severity, color, cformat, log_args);)
        return exception;
    }

    std::exception StaticLog::ExceptionV(LogSeverity severity, LogColor color, const char* format, va_list args)
    {
        if (!s_ActiveLogger.expired())
        {
            return s_ActiveLogger.lock()->ExceptionV(severity, color, format, args);
        }

        return std::runtime_error(format);
    }

    #undef PK_FORWAD_VARGS_FUNC
}
