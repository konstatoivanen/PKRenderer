#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    class PassFilmGrain : public NoCopy
    {
        public:
            PassFilmGrain(AssetDatabase* assetDatabase);
            void Compute(CommandBufferExt cmd);

        private:
            ShaderAsset* m_computeFilmGrain = nullptr;
            RHITextureRef m_filmGrainTexture;
    };
}