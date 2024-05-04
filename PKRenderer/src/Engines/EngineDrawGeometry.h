#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/IService.h"
#include "Rendering/RHI/Structs.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::ControlFlow, class Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering, struct RenderPipelineEvent)

namespace PK::Engines
{
    class EngineDrawGeometry : public Core::IService,
        public Core::ControlFlow::IStep<Rendering::RenderPipelineEvent*>
    {
    public:
        EngineDrawGeometry(ECS::EntityDatabase* entityDb, Core::ControlFlow::Sequencer* sequencer);
        virtual void Step(Rendering::RenderPipelineEvent* renderEvent) final;
    private:
        ECS::EntityDatabase* m_entityDb = nullptr;
        Core::ControlFlow::Sequencer* m_sequencer = nullptr;
        Rendering::RHI::FixedFunctionShaderAttributes m_gbufferAttribs{};
    };
}