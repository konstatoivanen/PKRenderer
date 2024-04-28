#pragma once
#include "Core/Services/IService.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/ControlFlow/IStep.h"

namespace PK::Core::ControlFlow
{
    struct RemoteProcessCommand
    {
        const char* executablePath;
        const char* arguments;
    };

    class RemoteProcessRunner : 
        public PK::Core::Services::IService,
        public IStep<RemoteProcessCommand*>
    {
        public:
            RemoteProcessRunner() {};

            void ExecuteRemoteProcess(const RemoteProcessCommand& command);

            virtual void Step(RemoteProcessCommand* command) final { ExecuteRemoteProcess(*command); }

            // @TODO add multithreading support
    };
}