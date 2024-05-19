#pragma once
#include "Graphics/Material.h"

namespace PK::ECS
{
    struct ComponentMaterials
    {
        std::vector<Graphics::MaterialTarget> materials;
        virtual ~ComponentMaterials() = default;
    };
}