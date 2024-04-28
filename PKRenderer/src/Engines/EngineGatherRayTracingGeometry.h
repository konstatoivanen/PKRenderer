#pragma once
#include "Utilities/ForwardDeclareUtility.h"
#include "Core/Services/IService.h"
#include "Core/ControlFlow/IStep.h"
#include "ECS/EGID.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering, struct RequestRayTracingGeometry)

namespace PK::Engines
{
    class EngineGatherRayTracingGeometry : public Core::Services::IService,
        public Core::ControlFlow::IStep<Rendering::RequestRayTracingGeometry*>
    {
        public:
            EngineGatherRayTracingGeometry(ECS::EntityDatabase* entityDb);
            virtual void Step(Rendering::RequestRayTracingGeometry* token) final;

        private:
            ECS::EntityDatabase* m_entityDb = nullptr;
            std::vector<ECS::EGID> m_entityViewEgids;
    };
}