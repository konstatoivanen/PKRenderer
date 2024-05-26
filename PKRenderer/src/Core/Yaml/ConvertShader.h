#pragma once
#include <yaml-cpp/yaml.h>
#include "Core/Rendering/RenderingFwd.h"

namespace YAML
{
    template<>
    struct convert<PK::ShaderAsset*>
    {
        static Node encode(const PK::ShaderAsset*& rhs);
        static bool decode(const Node& node, PK::ShaderAsset*& rhs);
    };
}