#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "App/Renderer/EntityCulling.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)

namespace PK::App
{
    class EngineEntityCull : 
        public IStep<RequestEntityCullFrustum*>,
        public IStep<RequestEntityCullCubeFaces*>,
        public IStep<RequestEntityCullCascades*>
    {
    public:
        EngineEntityCull(EntityDatabase* entityDb);
        virtual void Step(RequestEntityCullFrustum* request) final;
        virtual void Step(RequestEntityCullCubeFaces* request) final;
        virtual void Step(RequestEntityCullCascades* request) final;

    private:
        EntityDatabase* m_entityDb = nullptr;
        CulledEntityInfoList m_synchronousResults;
    };
}