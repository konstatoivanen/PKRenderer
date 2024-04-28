#include "PrecompiledHeader.h"
#include "Math/FunctionsIntersect.h"
#include "ECS/EntityViewScenePrimitive.h"
#include "ECS/EntityViewStaticMesh.h"
#include "Rendering/RequestRayTracingGeometry.h"
#include "EngineGatherRayTracingGeometry.h"

namespace PK::Engines
{
    using namespace PK::Math;
    using namespace PK::ECS;
    using namespace PK::Rendering;
    using namespace PK::Rendering::RHI::Objects;

    EngineGatherRayTracingGeometry::EngineGatherRayTracingGeometry(EntityDatabase* entityDb) : m_entityDb(entityDb)
    {
    }

    void EngineGatherRayTracingGeometry::Step(RequestRayTracingGeometry* request)
    {
        PK_THROW_ASSERT(request != nullptr && request->structure, "Invalid token supplied!");

        m_entityViewEgids.clear();

        auto entityViews = m_entityDb->Query<EntityViewScenePrimitive>((uint32_t)ECS::ENTITY_GROUPS::ACTIVE);
        auto mask = (request->mask | ScenePrimitiveFlags::Mesh | ScenePrimitiveFlags::RayTraceable);
        auto aabb = request->bounds;
        auto structure = request->structure;
        auto skipCulling = !request->useBounds;

        for (auto i = 0u; i < entityViews.count; ++i)
        {
            if ((entityViews[i].primitive->flags & mask) == mask && (skipCulling || Functions::IntersectAABB(aabb, entityViews[i].bounds->worldAABB)))
            {
                m_entityViewEgids.push_back(entityViews[i].GID);
            }
        }

        structure->BeginWrite(request->queue, (uint32_t)m_entityViewEgids.size());

        AccelerationStructureGeometryInfo geometry{};
        geometry.customIndex = 0u;

        for (const auto& egid : m_entityViewEgids)
        {
            auto entityView = m_entityDb->Query<EntityViewStaticMesh>(egid);

            for (const auto& material : entityView->materials->materials)
            {
                if (entityView->staticMesh->sharedMesh->TryGetAccelerationStructureGeometryInfo(material.submesh, &geometry))
                {
                    structure->AddInstance(geometry, entityView->transform->localToWorld);
                }
            }
        }

        structure->EndWrite();
    }
}
