#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedArena.h"
#include "Core/Math/FunctionsIntersect.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Math/FunctionsMatrix.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/RHI/Structs.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "EngineEntityCull.h"

namespace PK::App
{
    void EngineEntityCull::Step(IArena* frameArena, RequestEntityCullFrustum* request)
    {
        auto cullingMask = request->mask;
        auto cullingPlanes = Math::ExtractFrustrumPlanes(request->matrix, true);

        auto cullingRange = cullingPlanes.near.w + cullingPlanes.far.w;
        auto cullingInvRange = (float)(0xFFFF) / cullingRange;
        auto cullingMinDepth = cullingRange;
        auto cullingMaxDepth = 0.0f;

        auto entityViews = m_entityDb->Query<EntityViewScenePrimitive>((uint32_t)ENTITY_GROUPS::ACTIVE);
        auto entityInfos = frameArena->GetHead<CulledEntityInfo>();

        for (auto i = 0u; i < entityViews.count; ++i)
        {
            auto entityView = &entityViews[i];
            auto viewFlags = entityView->primitive->flags;

            if ((viewFlags & cullingMask) == cullingMask)
            {
                if ((viewFlags & ScenePrimitiveFlags::NeverCull) != 0 || Math::IntersectPlanesAABB(cullingPlanes.array_ptr(), 6, entityView->bounds->worldAABB))
                {
                    auto depth = Math::PlaneMaxDistanceToAABB(cullingPlanes.near, entityView->bounds->worldAABB);
                    auto fixedDepth = glm::min(0xFFFFu, (uint32_t)glm::max(0.0f, depth * cullingInvRange));
                    cullingMinDepth = glm::min(cullingMinDepth, depth);
                    cullingMaxDepth = glm::max(cullingMaxDepth, depth);
                    frameArena->Emplace<CulledEntityInfo>({ entityView->GID.entityID(), (uint16_t)fixedDepth, 0u });
                }
            }
        }

        request->outResults = { entityInfos, frameArena->GetHeadDelta(entityInfos) };
        request->outMinDepth = cullingMinDepth;
        request->outMaxDepth = cullingMaxDepth;
        request->outDepthRange = cullingMaxDepth - cullingMinDepth;
    }

    void EngineEntityCull::Step(IArena* frameArena, RequestEntityCullCubeFaces* request)
    {
        const float3 cubePlaneNormals[] = { {-1,1,0}, {1,1,0}, {1,0,1}, {1,0,-1}, {0,1,1}, {0,-1,1} };
        const float3 cubePlaneNormalsAbs[] = { {1,1,0}, {1,1,0}, {1,0,1}, {1,0,1}, {0,1,1}, {0,1,1} };

        auto cullingBoundsCenter = request->aabb.GetCenter();
        auto cullingBounds = request->aabb;
        auto cullingMask = request->mask;

        auto cullingRange = (float)request->aabb.GetExtents().length();
        auto cullingInvRange = (float)(0xFFFF) / cullingRange;
        auto cullingMinDepth = cullingRange;
        auto cullingMaxDepth = 0.0f;

        auto entityViews = m_entityDb->Query<EntityViewScenePrimitive>((uint32_t)ENTITY_GROUPS::ACTIVE);
        auto entityInfos = frameArena->GetHead<CulledEntityInfo>();

        for (auto i = 0u; i < entityViews.count; ++i)
        {
            auto entityView = &entityViews[i];
            auto viewFlags = entityView->primitive->flags;
            auto ignoreCulling = (viewFlags & ScenePrimitiveFlags::NeverCull) != 0;

            if ((viewFlags & cullingMask) == cullingMask)
            {
                auto entityBounds = entityView->bounds->worldAABB;

                if (ignoreCulling || Math::IntersectAABB(cullingBounds, entityBounds))
                {
                    auto entityOffset = entityBounds.GetCenter() - cullingBoundsCenter;
                    auto entityExtents = entityBounds.GetExtents();
                    bool rp[6], rn[6];

                    // Source: https://newq.net/dl/pub/s2015_shadows.pdf
                    for (uint32_t j = 0u; j < 6; ++j)
                    {
                        auto dist = glm::dot(entityOffset, cubePlaneNormals[j]);
                        auto radius = glm::dot(entityExtents, cubePlaneNormalsAbs[j]);
                        rp[j] = dist > -radius;
                        rn[j] = dist < +radius;
                    }

                    uint32_t isVisible = 0u;
                    isVisible |= (uint32_t)(rn[0] && rp[1] && rp[2] && rp[3] && entityBounds.max.x > cullingBoundsCenter.x) << PK_RHI_CUBE_FACE_RIGHT;
                    isVisible |= (uint32_t)(rp[0] && rn[1] && rn[2] && rn[3] && entityBounds.min.x < cullingBoundsCenter.x) << PK_RHI_CUBE_FACE_LEFT;
                    isVisible |= (uint32_t)(rp[0] && rp[1] && rp[4] && rn[5] && entityBounds.max.y > cullingBoundsCenter.y) << PK_RHI_CUBE_FACE_UP;
                    isVisible |= (uint32_t)(rn[0] && rn[1] && rn[4] && rp[5] && entityBounds.min.y < cullingBoundsCenter.y) << PK_RHI_CUBE_FACE_DOWN;
                    isVisible |= (uint32_t)(rp[2] && rn[3] && rp[4] && rp[5] && entityBounds.max.z > cullingBoundsCenter.z) << PK_RHI_CUBE_FACE_FRONT;
                    isVisible |= (uint32_t)(rn[2] && rp[3] && rn[4] && rn[5] && entityBounds.min.z < cullingBoundsCenter.z) << PK_RHI_CUBE_FACE_BACK;
                    isVisible |= ignoreCulling ? ~0u : 0u;

                    if (isVisible != 0u)
                    {
                        auto depth = Math::ExtentsSignedDistance(entityOffset, entityExtents);
                        auto fixedDepth = glm::min(0xFFFFu, (uint32_t)glm::max(0.0f, depth * cullingInvRange));
                        auto entityId = entityView->GID.entityID();

                        for (auto j = 0u; j < 6u; ++j)
                        {
                            if (isVisible & (1 << j))
                            {
                                cullingMinDepth = glm::min(cullingMinDepth, depth);
                                cullingMaxDepth = glm::max(cullingMaxDepth, depth);
                                frameArena->Emplace<CulledEntityInfo>({ entityId, (uint16_t)fixedDepth, (uint16_t)j });
                            }
                        }
                    }
                }
            }
        }

        request->outResults = { entityInfos, frameArena->GetHeadDelta(entityInfos) };
        request->outMinDepth = cullingMinDepth;
        request->outMaxDepth = cullingMaxDepth;
        request->outDepthRange = cullingMaxDepth - cullingMinDepth;
    }

    // Near plane is unkwown & based on this cull step.
    // Far plane can be shifted forwards based on the results of this cull step.
    void EngineEntityCull::Step(IArena* frameArena, RequestEntityCullCascades* request)
    {
        // Skip near plane eval
        const auto cullingCascadeTestPlaneCount = 5u;

        auto cullingMask = request->mask;
        auto cullingCascadeCount = request->count;
        auto cullingCascadePlanes = PK_STACK_ALLOC(FrustumPlanes, cullingCascadeCount);
        auto cullingViewPlanes = PK_STACK_ALLOC(float4, cullingCascadeCount);
        auto cullingMaxDepth = 0.0f;

        for (auto i = 0u; i < cullingCascadeCount; ++i)
        {
            cullingCascadePlanes[i] = Math::ExtractFrustrumPlanes(request->cascades[i], true);
            cullingMaxDepth = glm::max(cullingMaxDepth, cullingCascadePlanes[i].near.w + cullingCascadePlanes[i].far.w);

            // Check against cascade splits as well. 
            // this eliminates some draws that do not contribute to the sampled portion of the shadow map.
            // Cascades should be further optimized however, as now most of the texel density is wasted by the axis aligned rect fitting
            // @TODO A potential optimization would be to find the rotations for minimum bound cascade frustums.
            auto cascadeDirection = float3(cullingCascadePlanes[i].near.xyz);
            auto offsetSign = glm::dot(cascadeDirection, float3(request->viewForwardPlane.xyz)) < 0.0f ? 0 : 1;
            cullingViewPlanes[i] = request->viewForwardPlane;
            cullingViewPlanes[i].w -= request->viewZOffsets[i + offsetSign];
            cullingViewPlanes[i] *= offsetSign == 0 ? 1.0f : -1.0f;
        }

        auto cullingMinDepth = cullingMaxDepth;

        auto entityViews = m_entityDb->Query<EntityViewScenePrimitive>((uint32_t)ENTITY_GROUPS::ACTIVE);
        auto entityInfos = frameArena->GetHead<CulledEntityInfo>();

        for (auto i = 0u; i < entityViews.count; ++i)
        {
            auto entityView = &entityViews[i];
            auto viewFlags = entityView->primitive->flags;

            if ((viewFlags & cullingMask) == cullingMask)
            {
                auto ignoreCulling = (viewFlags & ScenePrimitiveFlags::NeverCull) != 0;
                auto entityBounds = entityView->bounds->worldAABB;
                auto isVisible = 0u;

                for (auto j = 0u; j < cullingCascadeCount; ++j)
                {
                    auto visibility = ignoreCulling ||
                        (Math::IntersectPlanesAABB(cullingCascadePlanes[j].array_ptr(), cullingCascadeTestPlaneCount, entityBounds) &&
                         Math::IntersectPlanesAABB(&cullingViewPlanes[j], 1u, entityBounds));

                    isVisible |= (uint32_t)visibility << j;
                }

                if (isVisible != 0u)
                {
                    auto entityId = entityView->GID.entityID();

                    for (auto j = 0u; j < cullingCascadeCount; ++j)
                    {
                        if ((isVisible & (1 << j)) != 0u)
                        {
                            auto minDistLocal = Math::PlaneMinDistanceToAABB(cullingCascadePlanes[j].near, entityBounds);
                            cullingMinDepth = glm::min(cullingMinDepth, minDistLocal);
                            frameArena->Emplace<CulledEntityInfo>({ entityId, Math::PackHalf(minDistLocal), (uint16_t)j });
                        }
                    }
                }
            }
        }

        // In case of 0 results this will also output 0 which should be taken into account by users.
        const auto culledCount = frameArena->GetHeadDelta(entityInfos);
        const auto cullingRange = cullingMaxDepth - cullingMinDepth;
        const auto cullingInvRange = (float)(0xFFFF) / cullingRange;

        for (auto i = 0u; i < culledCount; ++i)
        {
            auto& info = entityInfos[i];
            auto nearOffset = Math::UnPackHalf(info.depth);
            auto fixedDepth = glm::min(0xFFFFu, (uint32_t)((nearOffset - cullingMinDepth) * cullingInvRange));
            info.depth = fixedDepth;
        }

        request->outResults = { entityInfos, culledCount };
        request->outMinDepth = cullingMinDepth;
        request->outMaxDepth = cullingMaxDepth;
        request->outDepthRange = cullingRange;
    }
}
