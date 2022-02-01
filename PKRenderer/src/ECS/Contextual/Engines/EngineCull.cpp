#include "PrecompiledHeader.h"
#include "EngineCull.h"
#include "ECS/Contextual/EntityViews/BaseRenderableView.h"
#include "Math/FunctionsIntersect.h"

namespace PK::ECS::Engines
{
    using namespace ECS::EntityViews;
    using namespace ECS::Tokens;
    using namespace Rendering::Structs;
    using namespace Math;

    EngineCull::EngineCull(EntityDatabase* entityDb) : m_entityDb(entityDb)
    {
    }

    void EngineCull::Step(TokenCullFrustum* token)
    {
        auto invDepthRange = (float)(0xFFFF) / token->depthRange;
        auto results = token->results;
        auto planes = token->planes.planes;
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

            auto isVisible = (flags & RenderableFlags::Cullable) == 0 || Functions::IntersectPlanesAABB(planes, 6, cullable->bounds->worldAABB);

            if (!isVisible)
            {
                continue;
            }

            auto depth = Functions::PlaneMaxDistanceToAABB(planes[4], cullable->bounds->worldAABB) * invDepthRange;
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
            vis[PK_CUBE_FACE_LEFT]  = rp[0] && rn[1] && rn[2] && rn[3] && bounds.min.x < aabbCenter.x;
            vis[PK_CUBE_FACE_UP]    = rp[0] && rp[1] && rp[4] && rn[5] && bounds.max.y > aabbCenter.y;
            vis[PK_CUBE_FACE_DOWN]  = rn[0] && rn[1] && rn[4] && rp[5] && bounds.min.y < aabbCenter.y;
            vis[PK_CUBE_FACE_FRONT] = rp[2] && rn[3] && rp[4] && rp[5] && bounds.max.z > aabbCenter.z;
            vis[PK_CUBE_FACE_BACK]  = rn[2] && rp[3] && rn[4] && rn[5] && bounds.min.z < aabbCenter.z;

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
        auto count = token->count;
        auto visibilities = PK_STACK_ALLOC(bool, count);
        auto invDepthRange = (float)(0xFFFF) / token->depthRange;
        auto results = token->results;
        auto cascades = token->cascades;
        auto mask = token->mask;
        auto cullables = m_entityDb->Query<BaseRenderableView>((uint32_t)ECS::ENTITY_GROUPS::ACTIVE);

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
                isVisible |= visibilities[j] = !isCullable || Functions::IntersectPlanesAABB(cascades[j].planes, 6, bounds);
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
                    auto depth = Functions::PlaneMinDistanceToAABB(cascades[j].planes[4], bounds) * invDepthRange;
                    auto fixedDepth = glm::min(0xFFFFu, (uint32_t)glm::max(0.0f, depth));
                    results->Add(id, (uint16_t)fixedDepth, j);
                }
            }
        }
    }
    
}