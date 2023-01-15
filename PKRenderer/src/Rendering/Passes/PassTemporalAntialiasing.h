#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/CommandBuffer.h"

namespace PK::Rendering::Passes
{
    class PassTemporalAntialiasing : public Utilities::NoCopy
    {
        public:
            PassTemporalAntialiasing(Core::Services::AssetDatabase* assetDatabase, uint32_t initialWidth, uint32_t initialHeight);
            void Execute(Objects::CommandBuffer* cmd, Objects::RenderTexture* source, Structs::MemoryAccessFlags& lastAccess);

            constexpr Math::float4 GetJitter() const { return m_jitter; };

        private:
            const uint32_t JitterSampleCount = 16u;

            Objects::Shader* m_computeTAA = nullptr;
            Utilities::Ref<Objects::Texture> m_renderTarget;
            uint32_t m_historyLayerIndex = 0u;
    
            Math::float4 m_jitter = Math::PK_FLOAT4_ZERO;
            uint32_t m_jitterSampleIndex = 0u;
            float m_jitterSpread = 1.0f;
    };
}