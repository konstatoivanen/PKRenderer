#pragma once
#include "Utilities/NoCopy.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Renderer::Passes
{
    class PassTemporalAntialiasing : public Utilities::NoCopy
    {
        public:
            PassTemporalAntialiasing(Core::Assets::AssetDatabase* assetDatabase, uint32_t initialWidth, uint32_t initialHeight);
            void Render(Graphics::CommandBufferExt cmd, Graphics::Texture* source, Graphics::Texture* destination);

            constexpr Math::float4 GetJitter() const { return m_jitter; };

        private:
            const uint32_t JitterSampleCount = 16u;

            Graphics::Shader* m_computeTAA = nullptr;
            Graphics::TextureRef m_renderTarget;
            uint32_t m_historyLayerIndex = 0u;
    
            Math::float4 m_jitter = Math::PK_FLOAT4_ZERO;
            uint32_t m_jitterSampleIndex = 0u;
            float m_jitterSpread = 1.0f;
    };
}