#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/CommandBuffer.h"

namespace PK::Rendering::Passes
{
    class PassBloom : public Utilities::NoCopy
    {
        public:
            PassBloom(Core::Services::AssetDatabase* assetDatabase, uint32_t initialWidth, uint32_t initialHeight);
            void Render(Objects::CommandBuffer* cmd, Objects::RenderTexture* source);

            Objects::Texture* GetTexture() { return m_bloomTexture.get(); }

        private:
            Objects::Shader* m_computeBloom = nullptr;
            Utilities::Ref<Objects::Texture> m_bloomTexture;
            uint32_t m_passDownsample0 = 0;
            uint32_t m_passDownsample = 0;
            uint32_t m_passSeparableBlur = 0;
            uint32_t m_passUpsample = 0;
    };
}