#pragma once
#include "Math/Types.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::ECS::Tokens
{
    struct TokenAccelerationStructureBuild
    {
        Rendering::RHI::QueueType queue;
        Rendering::RHI::Objects::AccelerationStructure* structure;
        Rendering::Structs::RenderableFlags mask;
        Math::BoundingBox bounds;
        bool useBounds;

        TokenAccelerationStructureBuild(Rendering::RHI::QueueType queue,
            Rendering::RHI::Objects::AccelerationStructure* structure,
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