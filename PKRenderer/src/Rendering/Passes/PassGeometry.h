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
    using namespace PK::Core;
    using namespace PK::ECS::Tokens;
    using namespace PK::Rendering::Objects;

    class PassGeometry : public PK::Core::NoCopy
    {
        public:
            PassGeometry(EntityDatabase* entityDb, Sequencer* sequencer, Batcher* batcher);
            void Cull(void* engineRoot, VisibilityList* visibilityList, const float4x4& viewProjection, float depthRange);
            void RenderForward(CommandBuffer* cmd);
            void RenderGBuffer(CommandBuffer* cmd);
            constexpr uint32_t GetPassGroup() const { return m_passGroup; }
        private:
            EntityDatabase* m_entityDb = nullptr;
            Sequencer* m_sequencer = nullptr;
            Batcher* m_batcher = nullptr;
            uint32_t m_passGroup = 0u;
            FixedFunctionShaderAttributes m_gbufferAttribs{};
    };
}