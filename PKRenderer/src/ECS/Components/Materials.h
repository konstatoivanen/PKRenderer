#pragma once
#include "Rendering/Objects/Material.h"

namespace PK::ECS::Components
{
    struct Materials
    {
        std::vector<Rendering::Objects::MaterialTarget> materials;
        virtual ~Materials() = default;
    };
}