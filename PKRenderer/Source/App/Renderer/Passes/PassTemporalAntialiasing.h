#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    class PassTemporalAntialiasing : public NoCopy
    {
        public:
            struct ViewResources
            {
                RHITextureRef historyTexture;
            };

            PassTemporalAntialiasing(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void Render(CommandBufferExt cmd, RenderView* view, RHITexture* source, RHITexture* destination);
            constexpr float4 GetJitter() const { return m_jitter; };

        private:
            const uint32_t JITTER_SAMPLE_COUNT = 16u;

            ShaderAsset* m_computeTAA = nullptr;

            uint32_t m_historyLayerIndex = 0u;
            float4 m_jitter = PK_FLOAT4_ZERO;
            uint32_t m_jitterSampleIndex = 0u;
            float m_jitterSpread = 2.0f;
    };
}