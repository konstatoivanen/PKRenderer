#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    struct ApplicationConfig;

    class PassDepthOfField : public NoCopy
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
            PassDepthOfField(AssetDatabase* assetDatabase, const ApplicationConfig* config);
            void ComputeAutoFocus(CommandBufferExt cmd, uint32_t screenHeight);
            void Render(CommandBufferExt cmd, RHITexture* destination);
            void OnUpdateParameters(const ApplicationConfig* config);

        private:
            ShaderAsset* m_computeDepthOfField = nullptr;
            ShaderAsset* m_computeAutoFocus = nullptr;
            RHITextureRef m_colorTarget;
            RHITextureRef m_alphaTarget;
            RHIBufferRef m_autoFocusParams;
            uint32_t m_passPrefilter = 0u;
            uint32_t m_passDiskblur = 0u;
            uint32_t m_passUpsample = 0u;
            Constants m_constants{};
    };
}