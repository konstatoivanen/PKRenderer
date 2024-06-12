#include "PrecompiledHeader.h"
#include "Core/Utilities/RemoteProcess.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "RemoteProcessRunner.h"

namespace PK
{
    RemoteProcessRunner::RemoteProcessRunner()
    {
        // @TODO hmm not a good cvar namespace
        CVariableRegister::Create<CVariableFunc>("Application.Run.Executable", [this](const char** args, uint32_t count)
            {
                ExecuteRemoteProcess(args, count);
            }, 
            "executable, arguments", 1u, 256u);
    }

    void RemoteProcessRunner::ExecuteRemoteProcess(const char** args, uint32_t count)
    {
        auto combinedArguments = std::string();

        for (auto i = 1u; i < count; ++i)
        {
            combinedArguments.append(args[i]);
            combinedArguments.append(" ");
        }

        ExecuteRemoteProcess({ args[0], combinedArguments.c_str() });
    }

    void RemoteProcessRunner::ExecuteRemoteProcess(const RemoteProcessCommand& command)
    {
        PK_LOG_NEWLINE();
        PK_LOG_NEWLINE();
        PK_LOG_INFO("ExecuteRemoteProcess: %s  %s", command.executablePath, command.arguments);
        PK_LOG_INFO("----------REMOTE EXECUTE SCOPE----------");

        std::string error;
        if (!RemoteProcess::Execute(command.executablePath, command.arguments, error))
        {
            PK_LOG_INFO("----------REMOTE EXECUTE SCOPE----------");
            PK_LOG_INFO("ExecuteRemoteProcess: Failed to run %s %s", command.executablePath, command.arguments);
            PK_LOG_NEWLINE();
        }
        else
        {
            PK_LOG_NEWLINE();
            PK_LOG_INFO("----------REMOTE EXECUTE SCOPE----------");
            PK_LOG_INFO("ExecuteRemoteProcess: Finished running %s %s", command.executablePath, command.arguments);
            PK_LOG_NEWLINE();
        }
    }
}