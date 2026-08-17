#include "PrecompiledHeader.h"
#include "Core/Math/Bounds.h"
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
        auto views = m_entityDb->Query<EntityViewTransform>();

        for (auto& view : views)
        {
            view.transform->localToWorld = view.transform->GetLocalToWorld();
            view.transform->worldToLocal = math::affineInverseTranspose(view.transform->localToWorld);
            view.transform->minUniformScale = math::cmin(math::abs(view.transform->scale));
            view.bounds->worldAABB = math::mul(view.transform->localToWorld, view.bounds->localAABB);
        }
    }
}
