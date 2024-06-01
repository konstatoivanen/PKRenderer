#pragma once
#include <yaml-cpp/yaml.h>
#include "Core/Rendering/RenderingFwd.h"

namespace YAML
{
    template<>
    struct convert<PK::TextureAsset*>
    {
        static Node encode(const PK::TextureAsset*& rhs);
        static bool decode(const Node& node, PK::TextureAsset*& rhs);
    };
}