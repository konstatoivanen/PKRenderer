#pragma once
#include "Core/ControlFlow/IStep.h"
#include "Core/RHI/Structs.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK { struct Sequencer; }
namespace PK { struct EntityDatabase; }

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
