#pragma once
#include "Math/Types.h"
#include "Rendering/EntityEnums.h"
#include "Rendering/RHI/RHI.h"

namespace PK::Rendering
{
    struct RequestRayTracingGeometry
    {
        Rendering::RHI::QueueType queue;
        Rendering::RHI::Objects::AccelerationStructure* structure;
        Rendering::ScenePrimitiveFlags mask;
        Math::BoundingBox bounds;
        bool useBounds;

        RequestRayTracingGeometry(Rendering::RHI::QueueType queue,
            Rendering::RHI::Objects::AccelerationStructure* structure,
            Rendering::ScenePrimitiveFlags mask,
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