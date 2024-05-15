#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/RHI/RHI.h"

namespace PK::Rendering::Passes
{
    class PassTemporalAntialiasing : public Utilities::NoCopy
    {
        public:
            PassTemporalAntialiasing(Core::Assets::AssetDatabase* assetDatabase, uint32_t initialWidth, uint32_t initialHeight);
            void Render(RHI::Objects::CommandBuffer* cmd, RHI::Objects::Texture* source, RHI::Objects::Texture* destination);

            constexpr Math::float4 GetJitter() const { return m_jitter; };

        private:
            const uint32_t JitterSampleCount = 16u;

            RHI::Objects::Shader* m_computeTAA = nullptr;
            RHI::Objects::TextureRef m_renderTarget;
            uint32_t m_historyLayerIndex = 0u;
    
            Math::float4 m_jitter = Math::PK_FLOAT4_ZERO;
            uint32_t m_jitterSampleIndex = 0u;
            float m_jitterSpread = 1.0f;
    };
}