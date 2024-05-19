#pragma once
#include <yaml-cpp/yaml.h>
#include "Graphics/GraphicsFwd.h"

namespace YAML
{
    template<>
    struct convert<PK::Graphics::TextureAsset*>
    {
        static Node encode(const PK::Graphics::TextureAsset*& rhs);
        static bool decode(const Node& node, PK::Graphics::TextureAsset*& rhs);
    };
}