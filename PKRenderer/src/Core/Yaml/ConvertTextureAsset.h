#pragma once
#include <yaml-cpp/yaml.h>
#include "Rendering/Objects/TextureAsset.h"

namespace YAML
{
    template<>
    struct convert<PK::Rendering::Objects::TextureAsset*>
    {
        static Node encode(const PK::Rendering::Objects::TextureAsset*& rhs);
        static bool decode(const Node& node, PK::Rendering::Objects::TextureAsset*& rhs);
    };
}