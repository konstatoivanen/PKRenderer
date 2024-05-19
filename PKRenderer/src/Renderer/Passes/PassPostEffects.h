#pragma once
#include "Utilities/NoCopy.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Renderer::Passes
{
    class PassPostEffectsComposite : public PK::Utilities::NoCopy
    {
        public:
            PassPostEffectsComposite(Core::Assets::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void Render(Graphics::CommandBufferExt cmd, Graphics::Texture* destination);
            void OnUpdateParameters(PK::Core::Assets::AssetImportEvent<PK::Core::ApplicationConfig>* token);

        private:
            Graphics::Shader* m_computeComposite = nullptr;
            Graphics::ConstantBufferRef m_constantsPostProcess;
            Graphics::Texture* m_bloomLensDirtTexture;
            Graphics::Texture* m_colorgradingLut;
            uint32_t m_passIndex = 0u;
    };
}