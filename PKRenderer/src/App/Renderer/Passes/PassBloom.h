#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    class PassBloom : public NoCopy
    {
        public:
            PassBloom(AssetDatabase* assetDatabase, uint32_t initialWidth, uint32_t initialHeight);
            void Render(CommandBufferExt cmd, RHITexture* source);

            RHITexture* GetTexture();

        private:
            ShaderAsset* m_computeBloom = nullptr;
            RHITextureRef m_bloomTexture;
            uint32_t m_passDownsample0 = 0;
            uint32_t m_passDownsample = 0;
            uint32_t m_passSeparableBlur = 0;
            uint32_t m_passUpsample = 0;
    };
}