#include "PrecompiledHeader.h"
#include "Core/ControlFlow/Sequencer.h"
#include "EntityCulling.h"

namespace PK::App
{
    RequestEntityCullResults EntityCullSequencerProxy::CullFrustum(ScenePrimitiveFlags mask, const float4x4& matrix)
    {
        RequestEntityCullFrustum request;
        request.mask = mask;
        request.matrix = matrix;
        sequencer->Next(sequencerRoot, &request);
        return request;
    }

    RequestEntityCullResults EntityCullSequencerProxy::CullCubeFaces(ScenePrimitiveFlags mask, const BoundingBox& aabb)
    {
        RequestEntityCullCubeFaces request;
        request.mask = mask;
        request.aabb = aabb;
        sequencer->Next(sequencerRoot, &request);
        return request;
    }

    RequestEntityCullResults EntityCullSequencerProxy::CullCascades(ScenePrimitiveFlags mask, float4x4* cascades, const float4& viewForwardPlane, const float* viewZOffsets, uint32_t count)
    {
        RequestEntityCullCascades request;
        request.mask = mask;
        request.cascades = cascades;
        request.viewForwardPlane = viewForwardPlane;
        request.viewZOffsets = viewZOffsets;
        request.count = count;
        sequencer->Next(sequencerRoot, &request);
        return request;
    }

    void EntityCullSequencerProxy::CullRayTracingGeometry(ScenePrimitiveFlags mask, const BoundingBox& bounds, bool useBounds, QueueType queue, RHIAccelerationStructure* structure)
    {
        RequestEntityCullRayTracingGeometry request;
        request.mask = mask;
        request.bounds = bounds;
        request.useBounds = useBounds;
        request.queue = queue;
        request.structure = structure;
        sequencer->Next(sequencerRoot, &request);
    }
}