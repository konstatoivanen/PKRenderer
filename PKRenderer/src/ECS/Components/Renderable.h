#pragma once
#include "Rendering/Structs/Enums.h"

namespace PK::ECS::Components
{
    struct Renderable
    {
        PK::Rendering::Structs::RenderableFlags flags = PK::Rendering::Structs::RenderableFlags::Mesh;
        virtual ~Renderable() = default;
    };
}