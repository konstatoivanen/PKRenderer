#pragma once
#include "Core/ControlFlow/IStep.h"

namespace PK
{
    struct RemoteProcessCommand
    {
        const char* executablePath;
        const char* arguments;
    };

    class RemoteProcessRunner : public IStep<RemoteProcessCommand*>
    {
    public:
        RemoteProcessRunner();

        void ExecuteRemoteProcess(const char** args, uint32_t count);

        void ExecuteRemoteProcess(const RemoteProcessCommand& command);

        virtual void Step(RemoteProcessCommand* command) final { ExecuteRemoteProcess(*command); }

        // @TODO add multithreading support!? Currently blocks application controlflow.
    };
}