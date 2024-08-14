#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/CLI/ILogger.h"

namespace PK
{
    class LoggerPrintf : public ILogger, public NoCopy
    {
    public:
        LoggerPrintf() {}

        void Indent() final;
        void Unindent() final;

        void SetSeverityMask(LogSeverity mask) final;
        LogSeverity GetSeverityMask() const final;
        void SetColor(LogColor color) final;
        void SetShowConsole(bool value) final;

        void LogNewLine() final;
        void LogV(LogSeverity severity, LogColor color, const char* format, va_list args) final;
        void LogRewriteV(LogColor color, const char* format, va_list args) final;
        std::exception ExceptionV(LogSeverity severity, LogColor color, const char* format, va_list args) final;

    private:
        void LogIndent();
        void LogLineRemainder(int32_t linelength);

        constexpr static uint32_t MAX_INDENT = 256u;

        uint32_t m_severityMask = ~0;
        int32_t m_indentation = 0;
        int32_t m_lineClearLength = 0;
    };
}
