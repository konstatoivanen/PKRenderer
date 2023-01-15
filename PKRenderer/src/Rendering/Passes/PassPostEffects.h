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
    class PassPostEffects : public PK::Utilities::NoCopy
    {
        public:
            PassPostEffects(Core::Services::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void Render(Objects::CommandBuffer* cmd, Objects::RenderTexture* destination, Structs::MemoryAccessFlags lastAccess);
            void OnUpdateParameters(const Core::ApplicationConfig* config);
            void OnModifyProjection(PK::ECS::Tokens::ViewProjectionUpdateToken* token);

        private:
            PassBloom m_bloom;
            PassHistogram m_histogram;
            PassDepthOfField m_depthOfField;
            PassTemporalAntialiasing m_temporalAntialiasing;

            Objects::Shader* m_computeComposite = nullptr;
            Objects::Shader* m_computeFilmGrain = nullptr;
            Objects::Texture* m_bloomLensDirtTexture;
            Utilities::Ref<Objects::Texture> m_filmGrainTexture;
            Utilities::Ref<Objects::ConstantBuffer> m_paramatersBuffer;
    };
}