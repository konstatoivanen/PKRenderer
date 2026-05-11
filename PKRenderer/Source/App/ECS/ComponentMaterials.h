#pragma once
#include "Core/Utilities/ArrayList.h"
#include "Core/Rendering/Material.h"

namespace PK::App
{
    struct ComponentMaterials
    {
        InlineList<MaterialTarget, 1ull> materials;
        virtual ~ComponentMaterials() = default;
    };
}
