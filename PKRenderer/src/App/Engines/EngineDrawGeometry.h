#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/RHI/Structs.h"
#include "Core/Rendering/RenderingFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class Sequencer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)

namespace PK::App
{
    struct RenderPipelineEvent;

    class EngineDrawGeometry : public IStep<RenderPipelineEvent*>
    {
    public:
        EngineDrawGeometry(EntityDatabase* entityDb, Sequencer* sequencer);
        virtual void Step(RenderPipelineEvent* renderEvent) final;
    private:
        EntityDatabase* m_entityDb = nullptr;
        Sequencer* m_sequencer = nullptr;
        FixedFunctionShaderAttributes m_gbufferAttribs{};
    };
}