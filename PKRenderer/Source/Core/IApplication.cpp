#include "PrecompiledHeader.h"
#include "Utilities/NameIDProviderDefault.h"
#include "Core/Math/Random.h"
#include "CLI/CVariableRegister.h"
#include "CLI/Log.h"
#include "IApplication.h"

namespace PK
{
    IApplication::~IApplication() = default;

    IApplication::IApplication(const CArguments& arguments, const char* name, Ref<ILogger> logger) :
        m_arguments(arguments),
        m_name(name),
        m_workingDirectory(String::ToFilePathDirectoryLength(arguments.args[0]), arguments.args[0]),
        m_logger(logger)
    {
        StaticLog::SetLogger(m_logger);

        {
            PK_LOG_VERBOSE_SCOPE("Creating '%s' application with '%u' arguments:", name, arguments.count);

            for (auto i = 0; i < arguments.count; ++i)
            {
                PK_LOG_VERBOSE("%s", arguments.args[i]);
            }

            PK_LOG_NEWLINE();
        }

        m_services.Create<NameIDProviderDefault>();
        m_services.Create<CVariableRegister>();

        CVariableRegister::Create<CVariableFuncSimple>("Application.Close", [](){IApplication::Get()->Close();});

        CVariableRegister::Create<CVariableFunc>("Application.Log.Filter.RHI", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                StaticLog::SetSeverityMaskFlag(PK_LOG_LVL_RHI, String::To<bool>(args[0]));
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Log.Filter.VERBOSE", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                StaticLog::SetSeverityMaskFlag(PK_LOG_LVL_VERBOSE, String::To<bool>(args[0]));
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Log.Filter.INFO", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                StaticLog::SetSeverityMaskFlag(PK_LOG_LVL_INFO, String::To<bool>(args[0]));
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Log.Filter.WARNING", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                StaticLog::SetSeverityMaskFlag(PK_LOG_LVL_WARNING, String::To<bool>(args[0]));
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Log.Filter.ERROR", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                StaticLog::SetSeverityMaskFlag(PK_LOG_LVL_ERROR, String::To<bool>(args[0]));
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Log.ShowConsole", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                StaticLog::SetShowConsole(String::To<bool>(args[0]));
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Log.CrashLogPath", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                auto absolutePath = FixedString256({ IApplication::Get()->GetWorkingDirectory(), args[0] });
                StaticLog::SetCrashLogPath(absolutePath);
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Seed", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                math::setSeed(String::To<uint32_t>(args[0]));
            }, "Random seed value used for random number generation.", 1u);
    }
}