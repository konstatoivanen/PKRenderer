#pragma once
#include "Rendering/Structs/Enums.h"

namespace PK::ECS::Components
{
    using namespace PK::Rendering::Structs;

    struct Renderable
    {
        RenderableFlags flags = RenderableFlags::Mesh;
        virtual ~Renderable() = default;
    };
}