#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Passes/PassBloom.h"
#include "Rendering/Passes/PassHistogram.h"
#include "Rendering/Passes/PassDepthOfField.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Core;
    using namespace PK::Rendering::Objects;

    class PassPostEffects : public PK::Core::NoCopy
    {
        public:
            PassPostEffects(AssetDatabase* assetDatabase, const ApplicationConfig* config);
            void Render(CommandBuffer* cmd, RenderTexture* destination, MemoryAccessFlags lastAccess);
            void OnUpdateParameters(const ApplicationConfig* config);

        private:
            PassBloom m_bloom;
            PassHistogram m_histogram;
            PassDepthOfField m_depthOfField;

            Shader* m_computeComposite = nullptr;
            Shader* m_computeFilmGrain = nullptr;
            Texture* m_bloomLensDirtTexture;
            Ref<Texture> m_filmGrainTexture;
            Ref<ConstantBuffer> m_paramatersBuffer;
    };
}