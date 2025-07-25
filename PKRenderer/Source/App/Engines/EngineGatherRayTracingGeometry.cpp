#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsIntersect.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "App/ECS/EntityViewMeshStatic.h"
#include "App/Renderer/EntityCulling.h"
#include "EngineGatherRayTracingGeometry.h"

namespace PK::App
{
    EngineGatherRayTracingGeometry::EngineGatherRayTracingGeometry(EntityDatabase* entityDb) : m_entityDb(entityDb)
    {
    }

    void EngineGatherRayTracingGeometry::Step(RequestEntityCullRayTracingGeometry* request)
    {
        PK_DEBUG_THROW_ASSERT(request != nullptr && request->structure, "Invalid token supplied!");

        size_t instanceCount = 0u;

        // Testing all view types using the common primitive alias
        auto entityViews = m_entityDb->Query<EntityViewScenePrimitive>((uint32_t)ENTITY_GROUPS::ACTIVE);
        auto mask = (request->mask | ScenePrimitiveFlags::Mesh | ScenePrimitiveFlags::RayTraceable);
        auto aabb = request->bounds;
        auto structure = request->structure;
        auto skipCulling = !request->useBounds;

        for (auto i = 0u; i < entityViews.count; ++i)
        {
            auto& view = entityViews[i];
            view.primitive->isVisibleInRayTracing = (view.primitive->flags & mask) == mask && (skipCulling || Math::IntersectAABB(aabb, view.bounds->worldAABB));

            if (entityViews[i].primitive->isVisibleInRayTracing)
            {
                instanceCount++;
            }
        }

        structure->BeginWrite(request->queue, (uint32_t)instanceCount);

        AccelerationStructureGeometryInfo geometry{};
        geometry.customIndex = 0u;
        geometry.recordOffset = 0u;

        // Static scene mesh instances
        auto staticMeshViews = m_entityDb->Query<EntityViewMeshStatic>((uint32_t)ENTITY_GROUPS::ACTIVE);

        for (auto i = 0u; i < staticMeshViews.count; ++i)
        {
            auto& view = staticMeshViews[i];

            if (view.primitive->isVisibleInRayTracing)
            {
                for (const auto& material : view.materials->materials)
                {
                    if (view.staticMesh->sharedMesh->TryGetAccelerationStructureGeometryInfo(material->submesh, &geometry))
                    {
                        structure->AddInstance(geometry, view.transform->localToWorld);
                    }
                }
            }
        }

        structure->EndWrite();
    }
}
