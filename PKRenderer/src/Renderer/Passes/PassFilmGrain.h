#pragma once
#include "Utilities/NoCopy.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Renderer::Passes
{
    class PassFilmGrain : public PK::Utilities::NoCopy
    {
        public:
            PassFilmGrain(Core::Assets::AssetDatabase* assetDatabase);
            void Compute(Graphics::CommandBufferExt cmd);

        private:
            Graphics::Shader* m_computeFilmGrain = nullptr;
            Graphics::TextureRef m_filmGrainTexture;
    };
}