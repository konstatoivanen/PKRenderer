#pragma once
#include "Rendering/Objects/Material.h"

namespace PK::ECS::Components
{
    using namespace PK::Rendering::Objects;

    struct Materials
    {
        std::vector<Material*> sharedMaterials;
        virtual ~Materials() = default;
    };
}