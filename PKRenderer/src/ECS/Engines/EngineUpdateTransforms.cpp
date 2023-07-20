#include "PrecompiledHeader.h"
#include "EngineUpdateTransforms.h"
#include "ECS/EntityViews/TransformView.h"
#include "Math/FunctionsIntersect.h"

namespace PK::ECS::Engines
{
    using namespace PK::Math;

    EngineUpdateTransforms::EngineUpdateTransforms(EntityDatabase* entityDb)
    {
        m_entityDb = entityDb;
    }

    void EngineUpdateTransforms::Step(int condition)
    {
        auto views = m_entityDb->Query<EntityViews::TransformView>((int)ENTITY_GROUPS::ACTIVE);

        for (auto i = 0; i < views.count; ++i)
        {
            auto view = &views[i];
            view->transform->localToWorld = view->transform->GetLocalToWorld();
            view->transform->worldToLocal = glm::inverse(view->transform->localToWorld);
            view->bounds->worldAABB = Functions::BoundsTransform(view->transform->localToWorld, view->bounds->localAABB);
        }
    }
}