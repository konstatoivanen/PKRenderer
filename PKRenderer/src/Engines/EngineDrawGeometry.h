#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/IService.h"
#include "Graphics/RHI/Structs.h"
#include "Graphics/GraphicsFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::ControlFlow, class Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Renderer, struct RenderPipelineEvent)

namespace PK::Engines
{
    class EngineDrawGeometry : public Core::IService,
        public Core::ControlFlow::IStep<Renderer::RenderPipelineEvent*>
    {
    public:
        EngineDrawGeometry(ECS::EntityDatabase* entityDb, Core::ControlFlow::Sequencer* sequencer);
        virtual void Step(Renderer::RenderPipelineEvent* renderEvent) final;
    private:
        ECS::EntityDatabase* m_entityDb = nullptr;
        Core::ControlFlow::Sequencer* m_sequencer = nullptr;
        Graphics::RHI::FixedFunctionShaderAttributes m_gbufferAttribs{};
    };
}