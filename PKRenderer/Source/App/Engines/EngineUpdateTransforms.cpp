#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsIntersect.h"
#include "Core/ECS/EntityDatabase.h"
#include "App/ECS/EntityViewTransform.h"
#include "EngineUpdateTransforms.h"

namespace PK::App
{
    EngineUpdateTransforms::EngineUpdateTransforms(EntityDatabase* entityDb)
    {
        m_entityDb = entityDb;
    }

    void EngineUpdateTransforms::OnStepFrameUpdate([[maybe_unused]] FrameContext* ctx)
    {
        auto views = m_entityDb->Query<EntityViewTransform>((int)ENTITY_GROUPS::ACTIVE);

        for (auto i = 0u; i < views.count; ++i)
        {
            auto view = &views[i];
            view->transform->localToWorld = view->transform->GetLocalToWorld();
            view->transform->worldToLocal = Math::GetMatrixTransposeAffineInverse(view->transform->localToWorld);
            view->transform->minUniformScale = glm::compMin(glm::abs(view->transform->scale));
            view->bounds->worldAABB = Math::BoundsTransform(view->transform->localToWorld, view->bounds->localAABB);
        }
    }
}