#pragma once
#include "Core/Math/Math.h"
#include "Core/Yaml/ConvertMathTypes.h"
#include "Core/Yaml/ConfigMacros.h"
#include "Core/Yaml/ConvertCVariableCollection.h"

namespace PK::App
{
    PK_YAML_STRUCT_BEGIN(WindowConfig)
        PK_YAML_MEMBER(bool, Vsync, true)
        PK_YAML_MEMBER(bool, ShowCursor, true)
        PK_YAML_MEMBER(uint2, Size, uint2(1024u, 512u))
        PK_YAML_MEMBER(std::string, IconPath, "Content/T_AppIcon.bmp")
    PK_YAML_STRUCT_END()

    PK_YAML_ASSET_BEGIN(BaseRendererConfig, ".cfg")
        PK_YAML_MEMBER(float, TimeScale, 1.0f)
        PK_YAML_MEMBER_INLINE_STRUCT(WindowConfig, WindowDesc)
        PK_YAML_MEMBER(YAML::CVariableCollection, ConsoleVariables, {})
    PK_YAML_ASSET_END()
}

PK_YAML_ASSET_ASSETDATABSE_INTERFACE(PK::App::BaseRendererConfig)
