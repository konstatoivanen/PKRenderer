#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/Objects/CommandBuffer.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/RenderTexture.h"

namespace PK::Rendering::Passes
{
    class PassFilmGrain : public PK::Utilities::NoCopy
    {
        public:
            PassFilmGrain(Core::Services::AssetDatabase* assetDatabase);
            void Compute(Objects::CommandBuffer* cmd);

        private:
            Objects::Shader* m_computeFilmGrain = nullptr;
            Utilities::Ref<Objects::Texture> m_filmGrainTexture;
    };
}