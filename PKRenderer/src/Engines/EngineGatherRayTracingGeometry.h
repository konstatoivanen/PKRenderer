#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/IService.h"
#include "ECS/EGID.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Renderer, struct RequestEntityCullRayTracingGeometry)

namespace PK::Engines
{
    class EngineGatherRayTracingGeometry : public Core::IService,
        public Core::ControlFlow::IStep<Renderer::RequestEntityCullRayTracingGeometry*>
    {
    public:
        EngineGatherRayTracingGeometry(ECS::EntityDatabase* entityDb);
        virtual void Step(Renderer::RequestEntityCullRayTracingGeometry* token) final;

    private:
        ECS::EntityDatabase* m_entityDb = nullptr;
    };
}