#include "PrecompiledHeader.h"
#include <filesystem>
#include "Utilities/NameIDProviderDefault.h"
#include "Utilities/Parse.h"
#include "CLI/CVariableRegister.h"
#include "CLI/Log.h"
#include "IApplication.h"

namespace PK
{
    IApplication::~IApplication() = default;

    IApplication::IApplication(const CArguments& arguments, const char* name, Ref<ILogger> logger) :
        m_arguments(arguments),
        m_name(name),
        m_workingDirectory(std::filesystem::path(arguments.args[0]).remove_filename().string().c_str()),
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

        m_services = CreateUnique<ServiceRegister>();
        m_services->Create<NameIDProviderDefault>();
        m_services->Create<CVariableRegister>();

        CVariableRegister::Create<CVariableFuncSimple>("Application.Close", [](){IApplication::Get()->Close();});

        CVariableRegister::Create<CVariableFunc>("Application.Log.Filter.RHI", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                StaticLog::SetSeverityMaskFlag(PK_LOG_LVL_RHI, Parse::FromString<bool>(args[0]));
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Log.Filter.VERBOSE", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                StaticLog::SetSeverityMaskFlag(PK_LOG_LVL_VERBOSE, Parse::FromString<bool>(args[0]));
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Log.Filter.INFO", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                StaticLog::SetSeverityMaskFlag(PK_LOG_LVL_INFO, Parse::FromString<bool>(args[0]));
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Log.Filter.WARNING", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                StaticLog::SetSeverityMaskFlag(PK_LOG_LVL_WARNING, Parse::FromString<bool>(args[0]));
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Log.Filter.ERROR", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                StaticLog::SetSeverityMaskFlag(PK_LOG_LVL_ERROR, Parse::FromString<bool>(args[0]));
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Log.ShowConsole", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                StaticLog::SetShowConsole(Parse::FromString<bool>(args[0]));
            }, "0 = Off, 1 = On", 1u);

        CVariableRegister::Create<CVariableFunc>("Application.Seed", [](const char* const* args, [[maybe_unused]] uint32_t count)
            {
                srand(Parse::FromString<uint32_t>(args[0]));
            }, "Random seed value used for random number generation.", 1u);
    }
}