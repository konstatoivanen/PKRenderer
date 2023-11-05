#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::Rendering::Passes
{
    class PassFilmGrain : public PK::Utilities::NoCopy
    {
        public:
            PassFilmGrain(Core::Services::AssetDatabase* assetDatabase);
            void Compute(RHI::Objects::CommandBuffer* cmd);

        private:
            RHI::Objects::Shader* m_computeFilmGrain = nullptr;
            RHI::Objects::TextureRef m_filmGrainTexture;
    };
}