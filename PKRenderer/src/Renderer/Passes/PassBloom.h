#pragma once
#include "Utilities/NoCopy.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Renderer::Passes
{
    class PassBloom : public Utilities::NoCopy
    {
        public:
            PassBloom(Core::Assets::AssetDatabase* assetDatabase, uint32_t initialWidth, uint32_t initialHeight);
            void Render(Graphics::CommandBufferExt cmd, Graphics::Texture* source);

            Graphics::Texture* GetTexture();

        private:
            Graphics::Shader* m_computeBloom = nullptr;
            Graphics::TextureRef m_bloomTexture;
            uint32_t m_passDownsample0 = 0;
            uint32_t m_passDownsample = 0;
            uint32_t m_passSeparableBlur = 0;
            uint32_t m_passUpsample = 0;
    };
}