#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/FixedString.h"
#include "Core/CLI/ILogger.h"

namespace PK
{
    class LoggerPrintf : public ILogger, public NoCopy
    {
    public:
        LoggerPrintf() {}

        void SetCrashLogPath(const char* value) final;
        void SetSeverityMask(LogSeverity mask) final;
        LogSeverity GetSeverityMask() const final;
        void SetColor(LogColor color) final;
        void SetShowConsole(bool value) final;

        void Indent(LogSeverity severity) final;
        void Outdent(LogSeverity severity) final;
        void NewLine() final;
        void LogV(LogSeverity severity, LogColor color, const char* format, va_list args) final;
        std::exception ExceptionV(LogSeverity severity, LogColor color, const char* format, va_list args) final;

    private:
        void LogIndent();

        constexpr static uint32_t MAX_INDENT = 256u;

        FixedString256 m_crashLogPath;
        uint32_t m_severityMask = ~0;
        int32_t m_indentation[PK_LOG_LVL_COUNT]{};
    };
}
