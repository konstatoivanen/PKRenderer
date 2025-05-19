#pragma once
#include "Core/Math/Math.h"
#include "Core/CLI/CVariablesYaml.h"
#include "Core/Yaml/StructMacros.h"
#include "Core/Rendering/Window.h"
#include "Core/RHI/Structs.h"

namespace PK::App
{
    PK_YAML_STRUCT_BEGIN(RHISettings)
        PK_YAML_MEMBER(FixedString16, API, "Vulkan")
        PK_YAML_MEMBER(uint32_t, APIVersionMajor, 1)
        PK_YAML_MEMBER(uint32_t, APIVersionMinor, 4)
        PK_YAML_MEMBER(uint32_t, GCPruneDelay, 32)
        PK_YAML_MEMBER(bool, EnableValidation, true)
        PK_YAML_MEMBER(bool, EnableDebugNames, true)
        PK_YAML_MEMBER(bool, DiscardPipelineCache, false)
    PK_YAML_STRUCT_END()

    PK_YAML_ASSET_BEGIN(BaseRendererConfig, ".cfg")
        PK_YAML_MEMBER(float, TimeScale, 1.0f)
        PK_YAML_MEMBER(uint32_t, InactiveFrameInterval, 0u)
        PK_YAML_MEMBER(RHIDriverDescriptor, RHIDesc, {})
        PK_YAML_MEMBER(WindowDescriptor, WindowDesc, {})
        PK_YAML_MEMBER(CVariablesYaml, ConsoleVariables, {})
    PK_YAML_ASSET_END()
}

PK_YAML_ASSET_ASSETDATABSE_INTERFACE(PK::App::BaseRendererConfig)
