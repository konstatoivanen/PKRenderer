#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    class PassPostEffectsComposite : public NoCopy
    {
        public:
            PassPostEffectsComposite(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void Render(CommandBufferExt cmd, RHITexture* destination);

        private:
            ShaderAsset* m_computeComposite = nullptr;
            RHITexture* m_colorgradingLut = nullptr;
            RHITexture* m_tonemappingLut = nullptr;
            uint32_t m_passIndex = 0u;
    };
}