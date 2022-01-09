#pragma once
#include "Rendering/Objects/Material.h"

namespace PK::ECS::Components
{
    using namespace PK::Rendering::Objects;

    struct Materials
    {
        std::vector<MaterialTarget> materials;
        virtual ~Materials() = default;
    };
}