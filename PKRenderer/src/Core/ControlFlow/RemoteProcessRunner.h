#pragma once
#include "Core/ControlFlow/IStep.h"
#include "Core/IService.h"

namespace PK::Core::ControlFlow
{
    struct RemoteProcessCommand
    {
        const char* executablePath;
        const char* arguments;
    };

    class RemoteProcessRunner : public PK::Core::IService, public IStep<RemoteProcessCommand*>
    {
    public:
        RemoteProcessRunner() {};

        void ExecuteRemoteProcess(const RemoteProcessCommand& command);

        virtual void Step(RemoteProcessCommand* command) final { ExecuteRemoteProcess(*command); }

        // @TODO add multithreading support
    };
}