#include "PrecompiledHeader.h"
#include "EngineBuildAccelerationStructure.h"
#include "ECS/Contextual/EntityViews/BaseRenderableView.h"
#include "ECS/Contextual/EntityViews/MeshRenderableView.h"
#include "Math/FunctionsIntersect.h"

namespace PK::ECS::Engines
{
    using namespace PK::Rendering::Structs;
    using namespace PK::ECS::EntityViews;
    using namespace PK::Math;

    EngineBuildAccelerationStructure::EngineBuildAccelerationStructure(EntityDatabase* entityDb) : m_entityDb(entityDb)
    {
    }

    void EngineBuildAccelerationStructure::Step(Tokens::AccelerationStructureBuildToken* token)
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

        for (const auto& egid : m_renderableEgids)
        {
            auto renderable = m_entityDb->Query<MeshRenderableView>(egid);

            for (const auto& material : renderable->materials->materials)
            {
                structure->AddInstance(renderable->mesh->sharedMesh, material.submesh, 0u, renderable->transform->localToWorld);
            }
        }

        structure->EndWrite();
    }
}
