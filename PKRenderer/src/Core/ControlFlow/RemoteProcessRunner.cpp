#include "PrecompiledHeader.h"
#include "Utilities/RemoteProcess.h"
#include "Core/CLI/Log.h"
#include "RemoteProcessRunner.h"

namespace PK::Core::ControlFlow
{
    using namespace PK::Core::CLI;

    void RemoteProcessRunner::ExecuteRemoteProcess(const RemoteProcessCommand& command)
    {
        PK_LOG_NEWLINE();
        PK_LOG_NEWLINE();
        PK_LOG_INFO("ExecuteRemoteProcess: %s  %s", command.executablePath, command.arguments);
        PK_LOG_INFO("----------REMOTE EXECUTE SCOPE----------");

        std::string error;
        if (!Utilities::ExecuteRemoteProcess(command.executablePath, command.arguments, error))
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