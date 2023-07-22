#pragma once
#include "Core/Services/IService.h"
#include "Core/ApplicationConfig.h"
#include "ECS/Tokens/CullingTokens.h"
#include "ECS/Tokens/RenderingTokens.h"
#include "ECS/EntityDatabase.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Objects/Shader.h"

namespace PK::ECS::Engines
{
    class EngineDrawGeometry : public Core::Services::IService,
                               public Core::Services::IConditionalStep<Tokens::TokenRenderEvent>
    {
        public:
            EngineDrawGeometry(EntityDatabase* entityDb, Core::Services::Sequencer* sequencer);
            void Step(Tokens::TokenRenderEvent* token, int condition) final;
        private:
            ECS::EntityDatabase* m_entityDb = nullptr;
            Core::Services::Sequencer* m_sequencer = nullptr;
            Rendering::Structs::FixedFunctionShaderAttributes m_gbufferAttribs{};
            uint32_t m_passGroup = 0u;
    };
}