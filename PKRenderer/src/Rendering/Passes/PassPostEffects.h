#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Passes/PassBloom.h"
#include "Rendering/Passes/PassHistogram.h"
#include "Rendering/Passes/PassDepthOfField.h"
#include "Rendering/Passes/PassTemporalAntiAliasing.h"
#include "ECS/Contextual/Tokens/ViewProjectionToken.h"

namespace PK::Rendering::Passes
{
    class PassPostEffectsComposite : public PK::Utilities::NoCopy
    {
        public:
            PassPostEffectsComposite(Core::Services::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void Render(Objects::CommandBuffer* cmd, Objects::RenderTexture* destination);

        private:
            Objects::Shader* m_computeComposite = nullptr;
            Objects::Texture* m_bloomLensDirtTexture;
    };
}