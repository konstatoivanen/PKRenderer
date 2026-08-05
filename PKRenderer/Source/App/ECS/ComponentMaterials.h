#pragma once
#include "Core/Base/Containers/ArrayList.h"
#include "Core/Rendering/Material.h"

namespace PK::App
{
    struct ComponentMaterials
    {
        InlineList<MaterialTarget, 1ull> materials;
    };
}
