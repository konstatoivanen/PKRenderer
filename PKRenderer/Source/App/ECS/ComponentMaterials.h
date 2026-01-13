#pragma once
#include "Core/Utilities/FastList.h"
#include "Core/Rendering/Material.h"

namespace PK::App
{
    struct ComponentMaterials
    {
        FastList<MaterialTarget> materials;
        virtual ~ComponentMaterials() = default;
    };
}
