#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/IService.h"
#include "Renderer/EntityCulling.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)

namespace PK::Engines
{
    class EngineEntityCull : public Core::IService,
        public Core::ControlFlow::IStep<Renderer::RequestEntityCullFrustum*>,
        public Core::ControlFlow::IStep<Renderer::RequestEntityCullCubeFaces*>,
        public Core::ControlFlow::IStep<Renderer::RequestEntityCullCascades*>
    {
    public:
        EngineEntityCull(ECS::EntityDatabase* entityDb);
        virtual void Step(Renderer::RequestEntityCullFrustum* request) final;
        virtual void Step(Renderer::RequestEntityCullCubeFaces* request) final;
        virtual void Step(Renderer::RequestEntityCullCascades* request) final;

    private:
        ECS::EntityDatabase* m_entityDb = nullptr;
        Renderer::CulledEntityInfoList m_synchronousResults;
    };
}