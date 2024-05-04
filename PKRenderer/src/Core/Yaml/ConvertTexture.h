#pragma once
#include <yaml-cpp/yaml.h>
#include "Rendering/RHI/Objects/Texture.h"

namespace YAML
{
    template<>
    struct convert<PK::Rendering::RHI::Objects::Texture*>
    {
        static Node encode(const PK::Rendering::RHI::Objects::Texture*& rhs);
        static bool decode(const Node& node, PK::Rendering::RHI::Objects::Texture*& rhs);
    };
}