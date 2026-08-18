#include "PrecompiledHeader.h"
#include "Core/Math/Bounds.h"
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
        PK_DEBUG_FATAL_ASSERT(request != nullptr && request->structure, "Invalid token supplied!");

        auto instanceCount = 0u;

        // Testing all view types using the common primitive alias
        auto entityViews = m_entityDb->Query<EntityViewScenePrimitive>();
        auto mask = (request->mask | ScenePrimitiveFlags::Mesh | ScenePrimitiveFlags::RayTraceable);
        auto aabb = request->bounds;
        auto structure = request->structure;
        auto skipCulling = !request->useBounds;

        for (auto& view : entityViews)
        {
            view.primitive->isVisibleInRayTracing = (view.primitive->flags & mask) == mask && (skipCulling || math::intersects(aabb, view.bounds->worldAABB));

            if (view.primitive->isVisibleInRayTracing)
            {
                instanceCount++;
            }
        }

        structure->BeginWrite(request->queue, instanceCount);

        RayTracingGeometryInfo geometry{};
        geometry.customIndex = 0u;
        geometry.recordOffset = 0u;

        // Static scene mesh instances
        auto staticMeshViews = m_entityDb->Query<EntityViewMeshStatic>();

        for (auto& view : staticMeshViews)
        {
            if (view.primitive->isVisibleInRayTracing)
            {
                for (const auto& material : view.materials->materials)
                {
                    if (view.staticMesh->sharedMesh->GatherRayTracingGeometry(material.submesh, &geometry))
                    {
                        structure->AddInstance(geometry, view.transform->localToWorld);
                    }
                }
            }
        }

        structure->EndWrite();
    }
}
