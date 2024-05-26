#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    class PassTemporalAntialiasing : public NoCopy
    {
        public:
            PassTemporalAntialiasing(AssetDatabase* assetDatabase, uint32_t initialWidth, uint32_t initialHeight);
            void Render(CommandBufferExt cmd, RHITexture* source, RHITexture* destination);

            constexpr float4 GetJitter() const { return m_jitter; };

        private:
            const uint32_t JITTER_SAMPLE_COUNT = 16u;

            ShaderAsset* m_computeTAA = nullptr;
            RHITextureRef m_renderTarget;

            uint32_t m_historyLayerIndex = 0u;
            float4 m_jitter = PK_FLOAT4_ZERO;
            uint32_t m_jitterSampleIndex = 0u;
            float m_jitterSpread = 1.0f;
    };
}