#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Graphics/GraphicsFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)

namespace PK::Renderer::Passes
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
            PassDepthOfField(Core::Assets::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void ComputeAutoFocus(Graphics::CommandBufferExt cmd, uint32_t screenHeight);
            void Render(Graphics::CommandBufferExt cmd, Graphics::Texture* destination);
            void OnUpdateParameters(const Core::ApplicationConfig* config);

        private:
            Graphics::Shader* m_computeDepthOfField = nullptr;
            Graphics::Shader* m_computeAutoFocus = nullptr;
            Graphics::TextureRef m_colorTarget;
            Graphics::TextureRef m_alphaTarget;
            Graphics::BufferRef m_autoFocusParams;
            uint32_t m_passPrefilter = 0u;
            uint32_t m_passDiskblur = 0u;
            uint32_t m_passUpsample = 0u;
            Constants m_constants{};
    };
}