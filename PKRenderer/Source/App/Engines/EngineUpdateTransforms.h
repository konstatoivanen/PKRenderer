#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "App/FrameStep.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)

namespace PK::App
{
    class EngineUpdateTransforms : public IStepFrameUpdate<>
    {
    public:
        EngineUpdateTransforms(EntityDatabase* entityDb);
        virtual void OnStepFrameUpdate(FrameContext* ctx) final;

    private:
        EntityDatabase* m_entityDb = nullptr;
    };
}