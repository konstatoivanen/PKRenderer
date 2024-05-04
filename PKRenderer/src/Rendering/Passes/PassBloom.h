#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Rendering/RHI/Objects/Texture.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, struct CommandBuffer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Shader)

namespace PK::Rendering::Passes
{
    class PassBloom : public Utilities::NoCopy
    {
        public:
            PassBloom(Core::Assets::AssetDatabase* assetDatabase, uint32_t initialWidth, uint32_t initialHeight);
            void Render(RHI::Objects::CommandBuffer* cmd, RHI::Objects::Texture* source);

            RHI::Objects::Texture* GetTexture() { return m_bloomTexture.get(); }

        private:
            RHI::Objects::Shader* m_computeBloom = nullptr;
            RHI::Objects::TextureRef m_bloomTexture;
            uint32_t m_passDownsample0 = 0;
            uint32_t m_passDownsample = 0;
            uint32_t m_passSeparableBlur = 0;
            uint32_t m_passUpsample = 0;
    };
}