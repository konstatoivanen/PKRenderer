#include "PrecompiledHeader.h"
#include "Math/FunctionsIntersect.h"
#include "ECS/EntityViewTransform.h"
#include "EngineUpdateTransforms.h"

namespace PK::Engines
{
    using namespace PK::Math;
    using namespace PK::ECS;

    EngineUpdateTransforms::EngineUpdateTransforms(EntityDatabase* entityDb)
    {
        m_entityDb = entityDb;
    }

    void EngineUpdateTransforms::OnApplicationUpdateEngines()
    {
        auto views = m_entityDb->Query<EntityViewTransform>((int)ENTITY_GROUPS::ACTIVE);

        for (auto i = 0; i < views.count; ++i)
        {
            auto view = &views[i];
            view->transform->localToWorld= view->transform->GetLocalToWorld();
            view->transform->worldToLocal = Functions::GetMatrixTransposeAffineInverse(view->transform->localToWorld);
            view->bounds->worldAABB = Functions::BoundsTransform(view->transform->localToWorld, view->bounds->localAABB);
        }
    }
}