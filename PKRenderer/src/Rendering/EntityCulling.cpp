#include "PrecompiledHeader.h"
#include "Core/ControlFlow/Sequencer.h"
#include "EntityCulling.h"

namespace PK::Rendering
{
    using namespace PK::Math;

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
}