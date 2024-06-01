#pragma once
#include "Core/Rendering/Material.h"

namespace PK::App
{
    struct ComponentMaterials
    {
        std::vector<MaterialTarget> materials;
        virtual ~ComponentMaterials() = default;
    };
}