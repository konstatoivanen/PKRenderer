#pragma once
#include <yaml-cpp/yaml.h>
#include "Core/Input/InputKeyBinding.h"

namespace YAML
{
    template<>
    struct convert<PK::CommandInputKeyBindingMap>
    {
        static Node encode(const PK::CommandInputKeyBindingMap& rhs);
        static bool decode(const Node& node, PK::CommandInputKeyBindingMap& rhs);
    };

    template<>
    struct convert<PK::InputKeyCommandBindingMap>
    {
        static Node encode(const PK::InputKeyCommandBindingMap& rhs);
        static bool decode(const Node& node, PK::InputKeyCommandBindingMap& rhs);
    };
}