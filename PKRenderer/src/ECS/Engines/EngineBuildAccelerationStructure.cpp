#include "PrecompiledHeader.h"
#include "Math/FunctionsIntersect.h"
#include "ECS/EntityViews/BaseRenderableView.h"
#include "ECS/EntityViews/StaticMeshRenderableView.h"
#include "EngineBuildAccelerationStructure.h"

namespace PK::ECS::Engines
{
    using namespace PK::Math;
    using namespace PK::ECS::EntityViews;
    using namespace PK::Rendering::Structs;
    using namespace PK::Rendering::RHI::Objects;

    EngineBuildAccelerationStructure::EngineBuildAccelerationStructure(EntityDatabase* entityDb) : m_entityDb(entityDb)
    {
    }

    void EngineBuildAccelerationStructure::Step(Tokens::TokenAccelerationStructureBuild* token)
    {
        PK_THROW_ASSERT(token != nullptr && token->structure, "Invalid token supplied!");

        m_renderableEgids.clear();

        auto renderables = m_entityDb->Query<BaseRenderableView>((uint32_t)ECS::ENTITY_GROUPS::ACTIVE);
        auto mask = token->mask;
        auto aabb = token->bounds;
        auto structure = token->structure;
        auto requiredFlags = (RenderableFlags::Mesh | RenderableFlags::RayTraceable);

        for (auto i = 0u; i < renderables.count; ++i)
        {
            auto renderable = &renderables[i];
            auto flags = renderable->renderable->flags;

            if ((flags & requiredFlags) != requiredFlags ||
                (flags & mask) != mask)
            {
                continue;
            }

            auto bounds = renderable->bounds->worldAABB;

            if (token->useBounds && !Functions::IntersectAABB(aabb, bounds))
            {
                continue;
            }

            m_renderableEgids.push_back(renderable->GID);
        }

        structure->BeginWrite(token->queue, (uint32_t)m_renderableEgids.size());

        AccelerationStructureGeometryInfo geometry{};
        geometry.customIndex = 0u;

        for (const auto& egid : m_renderableEgids)
        {
            auto renderable = m_entityDb->Query<StaticMeshRenderableView>(egid);

            for (const auto& material : renderable->materials->materials)
            {
                if (renderable->staticMesh->sharedMesh->TryGetAccelerationStructureGeometryInfo(material.submesh, &geometry))
                {
                    structure->AddInstance(geometry, renderable->transform->localToWorld);
                }
            }
        }

        structure->EndWrite();
    }
}
