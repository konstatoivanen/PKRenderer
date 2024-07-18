#pragma once
#include <yaml-cpp/yaml.h>
#include "Core/Input/InputKeyBinding.h"

namespace YAML
{
    struct CVariableCollection {};

    template<>
    struct convert<CVariableCollection>
    {
        static Node encode(const CVariableCollection& rhs);
        static bool decode(const Node& node, CVariableCollection& rhs);
    };
}