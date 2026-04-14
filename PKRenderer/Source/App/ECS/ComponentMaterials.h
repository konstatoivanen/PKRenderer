#pragma once
#include "Core/Utilities/List.h"
#include "Core/Rendering/Material.h"

namespace PK::App
{
    struct ComponentMaterials
    {
        List<MaterialTarget, 1ull> materials;
        virtual ~ComponentMaterials() = default;
    };
}
