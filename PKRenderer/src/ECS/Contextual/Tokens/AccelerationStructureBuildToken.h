#pragma once
#include "Math/Types.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Objects/AccelerationStructure.h"

namespace PK::ECS::Tokens
{
    struct AccelerationStructureBuildToken
    {
        Rendering::Objects::AccelerationStructure* structure = 0;
        Rendering::Structs::RenderableFlags mask = (Rendering::Structs::RenderableFlags)0;
        Math::BoundingBox bounds{};
        bool useBounds = false;
    };
}