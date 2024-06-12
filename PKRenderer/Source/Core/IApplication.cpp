#include "PrecompiledHeader.h"
#include <filesystem>
#include "Utilities/NameIDProviderDefault.h"
#include "CLI/CVariableRegister.h"
#include "CLI/Log.h"
#include "IApplication.h"

namespace PK
{
    IApplication::~IApplication() = default;

    IApplication::IApplication(const CArguments& arguments, const std::string& name, Ref<ILogger> logger) :
        m_arguments(arguments),
        m_name(name),
        m_workingDirectory(std::filesystem::path(arguments.args[0]).remove_filename().string()),
        m_logger(logger)
    {
        StaticLog::SetLogger(m_logger);

        {
            PK_LOG_VERBOSE("Creating '%s' application with '%u' arguments:", name.c_str(), arguments.count);
            PK_LOG_SCOPE_INDENT(arguments);

            for (auto i = 0; i < arguments.count; ++i)
            {
                PK_LOG_VERBOSE("%s", arguments.args[i]);
            }

            PK_LOG_NEWLINE();
        }

        m_services = CreateScope<ServiceRegister>();
        m_services->Create<NameIDProviderDefault>();
        m_services->Create<CVariableRegister>();

        CVariableRegister::Create<CVariableFuncSimple>("Application.Close", [](){IApplication::Get()->Close();});
    }
}