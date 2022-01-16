#pragma once
#include <cstdint>

namespace PK::Rendering::Structs
{
    struct ExecutionGate
    {
        uint64_t invocationIndex = 0;
        const uint64_t* remoteInvocationIndex = nullptr;
        inline void Invalidate() { remoteInvocationIndex = nullptr; }
        inline bool IsValid() const { return remoteInvocationIndex != nullptr; }
        inline bool IsCompleted() const { return remoteInvocationIndex == nullptr || *remoteInvocationIndex != invocationIndex; }
    };
}