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

    template<>
    struct convert<PK::RHITexture*>
    {
        static Node encode(const PK::RHITexture*& rhs);
        static bool decode(const Node& node, PK::RHITexture*& rhs);
    };
}