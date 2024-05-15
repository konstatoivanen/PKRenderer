#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/RHI/RHI.h"

namespace PK::Rendering::Passes
{
    class PassBloom : public Utilities::NoCopy
    {
        public:
            PassBloom(Core::Assets::AssetDatabase* assetDatabase, uint32_t initialWidth, uint32_t initialHeight);
            void Render(RHI::Objects::CommandBuffer* cmd, RHI::Objects::Texture* source);

            RHI::Objects::Texture* GetTexture();

        private:
            RHI::Objects::Shader* m_computeBloom = nullptr;
            RHI::Objects::TextureRef m_bloomTexture;
            uint32_t m_passDownsample0 = 0;
            uint32_t m_passDownsample = 0;
            uint32_t m_passSeparableBlur = 0;
            uint32_t m_passUpsample = 0;
    };
}