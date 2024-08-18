#pragma once
#include "Core/Utilities/MemoryBlock.h"
#include "Core/Rendering/Material.h"

namespace PK::App
{
    struct ComponentMaterials
    {
        MemoryBlock<MaterialTarget> materials;
        virtual ~ComponentMaterials() = default;
    };
}