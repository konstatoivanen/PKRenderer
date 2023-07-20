#pragma once
#include "Math/Types.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Objects/AccelerationStructure.h"

namespace PK::ECS::Tokens
{
    struct TokenAccelerationStructureBuild
    {
        Rendering::Structs::QueueType queue;
        Rendering::Objects::AccelerationStructure* structure;
        Rendering::Structs::RenderableFlags mask;
        Math::BoundingBox bounds;
        bool useBounds;

        TokenAccelerationStructureBuild(Rendering::Structs::QueueType queue,
            Rendering::Objects::AccelerationStructure* structure,
            Rendering::Structs::RenderableFlags mask,
            Math::BoundingBox bounds,
            bool useBounds) :
            queue(queue),
            structure(structure),
            mask(mask),
            bounds(bounds),
            useBounds(useBounds)
        {
        }
    };
}