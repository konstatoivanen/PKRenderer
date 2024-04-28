#pragma once
#include "Rendering/Objects/Material.h"

namespace PK::ECS
{
    struct ComponentMaterials
    {
        std::vector<Rendering::Objects::MaterialTarget> materials;
        virtual ~ComponentMaterials() = default;
    };
}