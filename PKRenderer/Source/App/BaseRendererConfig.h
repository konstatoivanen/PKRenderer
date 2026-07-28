#pragma once
#include "Core/CLI/CVariablesYaml.h"
#include "Core/Rendering/Window.h"
#include "Core/RHI/Structs.h"

namespace PK::App
{
    struct BaseRendererConfig
    {
        float TimeScale = 1.0f;
        uint32_t InactiveFrameInterval = 0u;
        RHIDriverDescriptor RHIDesc = {};
        WindowDescriptor WindowDesc = {};
        CVariablesYaml ConsoleVariables = {};
    };
}
