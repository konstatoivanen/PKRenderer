#include "PrecompiledHeader.h"
#include "Math/FunctionsIntersect.h"
#include "Math/FunctionsMisc.h"
#include "Math/FunctionsMatrix.h"
#include "ECS/EntityViews/BaseRenderableView.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "EngineCull.h"

namespace PK::ECS::Engines
{
    using namespace PK::Math;
    using namespace PK::ECS::EntityViews;
    using namespace PK::ECS::Tokens;
    using namespace PK::Rendering::Structs;
    using namespace PK::Rendering::RHI;

    EngineCull::EngineCull(EntityDatabase* entityDb) : m_entityDb(entityDb)
    {
    }

    void EngineCull::Step(TokenCullFrustum* token)
    {
        auto invDepthRange = (float)(0xFFFF) / token->depthRange;
        auto results = token->results;
        auto mask = token->mask;
        auto cullables = m_entityDb->Query<BaseRenderableView>((uint32_t)ECS::ENTITY_GROUPS::ACTIVE);
        auto planes = Functions::ExtractFrustrumPlanes(token->matrix, true);

        for (auto i = 0u; i < cullables.count; ++i)
        {
            auto cullable = &cullables[i];
            auto flags = cullable->renderable->flags;

            if ((flags & mask) != mask)
            {
                continue;
            }

            auto isVisible = (flags & RenderableFlags::Cullable) == 0 || Functions::IntersectPlanesAABB(planes.array_ptr(), 6, cullable->bounds->worldAABB);

            if (!isVisible)
            {
                continue;
            }

            auto depth = Functions::PlaneMaxDistanceToAABB(planes.near, cullable->bounds->worldAABB) * invDepthRange;
            auto fixedDepth = glm::min(0xFFFFu, (uint32_t)glm::max(0.0f, depth));
            results->Add(cullable->GID.entityID(), (uint16_t)fixedDepth, 0u);
        }
    }

    void EngineCull::Step(TokenCullCubeFaces* token)
    {
        const float3 planeNormals[] = { {-1,1,0}, {1,1,0}, {1,0,1}, {1,0,-1}, {0,1,1}, {0,-1,1} };
        const float3 absPlaneNormals[] = { {1,1,0}, {1,1,0}, {1,0,1}, {1,0,1}, {0,1,1}, {0,1,1} };

        auto invDepthRange = (float)(0xFFFF) / token->depthRange;
        auto results = token->results;
        auto aabbCenter = token->aabb.GetCenter();
        auto aabb = token->aabb;
        auto mask = token->mask;
        auto cullables = m_entityDb->Query<BaseRenderableView>((uint32_t)ECS::ENTITY_GROUPS::ACTIVE);

        for (auto i = 0u; i < cullables.count; ++i)
        {
            auto cullable = &cullables[i];
            auto flags = cullable->renderable->flags;

            if ((flags & mask) != mask)
            {
                continue;
            }

            auto bounds = cullable->bounds->worldAABB;

            if (!Functions::IntersectAABB(aabb, bounds))
            {
                continue;
            }

            auto center = bounds.GetCenter() - aabbCenter;
            auto extents = bounds.GetExtents();

            // Not accurate but fast(er than other solutions)
            auto depth = glm::min(0xFFFFu, (uint32_t)(glm::length(center) * invDepthRange));

            bool rp[6], rn[6], vis[6];

            // Source: https://newq.net/dl/pub/s2015_shadows.pdf
            for (uint32_t j = 0u; j < 6; ++j)
            {
                auto dist = glm::dot(center, planeNormals[j]);
                auto radius = glm::dot(extents, absPlaneNormals[j]);
                rp[j] = dist > -radius;
                rn[j] = dist < radius;
            }

            vis[PK_CUBE_FACE_RIGHT] = rn[0] && rp[1] && rp[2] && rp[3] && bounds.max.x > aabbCenter.x;
            vis[PK_CUBE_FACE_LEFT] = rp[0] && rn[1] && rn[2] && rn[3] && bounds.min.x < aabbCenter.x;
            vis[PK_CUBE_FACE_UP] = rp[0] && rp[1] && rp[4] && rn[5] && bounds.max.y > aabbCenter.y;
            vis[PK_CUBE_FACE_DOWN] = rn[0] && rn[1] && rn[4] && rp[5] && bounds.min.y < aabbCenter.y;
            vis[PK_CUBE_FACE_FRONT] = rp[2] && rn[3] && rp[4] && rp[5] && bounds.max.z > aabbCenter.z;
            vis[PK_CUBE_FACE_BACK] = rn[2] && rp[3] && rn[4] && rn[5] && bounds.min.z < aabbCenter.z;

            auto isCullable = (flags & RenderableFlags::Cullable) != 0;
            auto id = cullable->GID.entityID();

            for (auto j = 0u; j < 6u; ++j)
            {
                if (!isCullable || vis[j])
                {
                    results->Add(id, (uint16_t)depth, j);
                }
            }
        }
    }

    void EngineCull::Step(TokenCullCascades* token)
    {
        // Near plane is unkwown & based on this cull step.
        // Far plane can be shifted forwards based on the results of this cull step.
        auto count = token->count;
        auto visibilities = PK_STACK_ALLOC(bool, count);
        auto results = token->results;
        auto cascades = token->cascades;
        auto mask = token->mask;
        auto cullables = m_entityDb->Query<BaseRenderableView>((uint32_t)ECS::ENTITY_GROUPS::ACTIVE);
        auto planes = PK_STACK_ALLOC(FrustumPlanes, count);
        // Skip near plane eval
        const auto planeCount = 5;

        auto maxFar = 0.0f;

        for (auto i = 0u; i < count; ++i)
        {
            planes[i] = Functions::ExtractFrustrumPlanes(token->cascades[i], true);
            maxFar = glm::max(maxFar, planes[i].near.w + planes[i].far.w);
        }

        auto minDist = maxFar;

        for (auto i = 0; i < cullables.count; ++i)
        {
            auto cullable = &cullables[i];
            auto flags = cullable->renderable->flags;

            if ((flags & mask) != mask)
            {
                continue;
            }

            auto isVisible = false;
            auto isCullable = (flags & RenderableFlags::Cullable) != 0;
            auto bounds = cullable->bounds->worldAABB;

            for (auto j = 0u; j < count; ++j)
            {
                isVisible |= visibilities[j] = !isCullable || Functions::IntersectPlanesAABB(planes[j].array_ptr(), planeCount, bounds);
            }

            if (!isVisible)
            {
                continue;
            }

            auto id = cullable->GID.entityID();

            for (auto j = 0u; j < count; ++j)
            {
                if (visibilities[j])
                {
                    auto minDistLocal = Functions::PlaneMinDistanceToAABB(planes[j].near, bounds);
                    minDist = glm::min(minDist, minDistLocal);
                    
                    // Need to do another loop to calculate this based on min-max.
                    results->Add(id, Functions::PackHalf(minDistLocal), j);
                }
            }
        }
        
        auto depthRange = maxFar - minDist;
        auto invDepthRange = (float)(0xFFFF) / depthRange;

        for (auto i = 0u; i < results->count; ++i)
        {
            auto nearOffset = Functions::UnPackHalf(results->GetAt(i).depth);
            auto fixedDepth = glm::min(0xFFFFu, (uint32_t)((nearOffset - minDist) * invDepthRange));
            results->GetAt(i).depth = fixedDepth;
        }

        token->depthRange = depthRange;
        token->outNearOffset = minDist;
    }
}