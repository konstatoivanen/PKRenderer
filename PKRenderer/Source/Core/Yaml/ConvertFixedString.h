#pragma once
#include <yaml-cpp/yaml.h>
#include "Core/Utilities/FixedString.h"

namespace YAML
{
    template<size_t capacity>
    struct convert<PK::FixedString<capacity>>
    {
        static Node encode(const PK::FixedString<capacity>& rhs)
        {
            Node node;
            node.push_back(std::string(rhs.c_str());
            return node;
        }

        static bool decode(const Node& node, PK::FixedString<capacity>& rhs)
        {
            rhs = PK::FixedString<capacity>(node.as<std::string>().c_str());
            return true;
        }
    };
}