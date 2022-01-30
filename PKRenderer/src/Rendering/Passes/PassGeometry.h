#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "ECS/Contextual/Tokens/CullingTokens.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Batcher.h"

namespace PK::Rendering::Passes
{
    class PassGeometry : public PK::Utilities::NoCopy
    {
        public:
            PassGeometry(ECS::EntityDatabase* entityDb, Core::Services::Sequencer* sequencer, Batcher* batcher);
            void Cull(void* engineRoot, ECS::Tokens::VisibilityList* visibilityList, const Math::float4x4& viewProjection, float depthRange);
            void RenderForward(Objects::CommandBuffer* cmd);
            void RenderGBuffer(Objects::CommandBuffer* cmd);
            constexpr uint32_t GetPassGroup() const { return m_passGroup; }
        private:
            ECS::EntityDatabase* m_entityDb = nullptr;
            Core::Services::Sequencer* m_sequencer = nullptr;
            Batcher* m_batcher = nullptr;
            uint32_t m_passGroup = 0u;
            Structs::FixedFunctionShaderAttributes m_gbufferAttribs{};
    };
}