#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Objects/Shader.h"

namespace PK::Rendering::Passes
{
    class PassDepthOfField : public PK::Utilities::NoCopy
    {
        struct Constants
        {
            float pk_DoF_FocalLength;
            float pk_DoF_FNumber;
            float pk_DoF_FilmHeight;
            float pk_DoF_FocusSpeed;
            float pk_DoF_MaximumCoC;
        };

        public:
            PassDepthOfField(Core::Services::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void ComputeAutoFocus(Objects::CommandBuffer* cmd, uint32_t screenHeight);
            void Render(Objects::CommandBuffer* cmd, Objects::RenderTexture* destination);
            void OnUpdateParameters(const Core::ApplicationConfig* config);

        private:
            Objects::Shader* m_computeDepthOfField = nullptr;
            Objects::Shader* m_computeAutoFocus = nullptr;
            Utilities::Ref<Objects::Texture> m_colorTarget;
            Utilities::Ref<Objects::Texture> m_alphaTarget;
            Utilities::Ref<Objects::Buffer> m_autoFocusParams;
            uint32_t m_passPrefilter = 0u;
            uint32_t m_passDiskblur = 0u;
            uint32_t m_passUpsample = 0u;
            Constants m_constants{};
    };
}