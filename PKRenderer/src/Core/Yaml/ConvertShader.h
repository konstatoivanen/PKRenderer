#pragma once
#include <yaml-cpp/yaml.h>
#include "Graphics/GraphicsFwd.h"

namespace YAML
{
    template<>
    struct convert<PK::Graphics::Shader*>
    {
        static Node encode(const PK::Graphics::Shader*& rhs);
        static bool decode(const Node& node, PK::Graphics::Shader*& rhs);
    };
}