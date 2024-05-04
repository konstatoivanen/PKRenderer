#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/IService.h"
#include "Rendering/EntityCulling.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)

namespace PK::Engines
{
    class EngineEntityCull : public Core::IService,
        public Core::ControlFlow::IStep<Rendering::RequestEntityCullFrustum*>,
        public Core::ControlFlow::IStep<Rendering::RequestEntityCullCubeFaces*>,
        public Core::ControlFlow::IStep<Rendering::RequestEntityCullCascades*>
    {
    public:
        EngineEntityCull(ECS::EntityDatabase* entityDb);
        virtual void Step(Rendering::RequestEntityCullFrustum* request) final;
        virtual void Step(Rendering::RequestEntityCullCubeFaces* request) final;
        virtual void Step(Rendering::RequestEntityCullCascades* request) final;

    private:
        ECS::EntityDatabase* m_entityDb = nullptr;

        Rendering::CulledEntityInfoList m_synchronousResults;
    };
}