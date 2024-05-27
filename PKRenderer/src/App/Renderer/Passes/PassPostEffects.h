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
            void Render(CommandBufferExt cmd, RHITexture* destination);
            void OnUpdateParameters(AssetImportEvent<ApplicationConfig>* token);

        private:
            ShaderAsset* m_computeComposite = nullptr;
            ConstantBufferRef m_constantsPostProcess;
            RHITexture* m_bloomLensDirtTexture;
            RHITexture* m_colorgradingLut;
            uint32_t m_passIndex = 0u;
    };
}