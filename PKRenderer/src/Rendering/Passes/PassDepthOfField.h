#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/RHI/GraphicsAPI.h"

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
            void ComputeAutoFocus(RHI::Objects::CommandBuffer* cmd, uint32_t screenHeight);
            void Render(RHI::Objects::CommandBuffer* cmd, RHI::Objects::Texture* destination);
            void OnUpdateParameters(const Core::ApplicationConfig* config);

        private:
            RHI::Objects::Shader* m_computeDepthOfField = nullptr;
            RHI::Objects::Shader* m_computeAutoFocus = nullptr;
            RHI::Objects::TextureRef m_colorTarget;
            RHI::Objects::TextureRef m_alphaTarget;
            RHI::Objects::BufferRef m_autoFocusParams;
            uint32_t m_passPrefilter = 0u;
            uint32_t m_passDiskblur = 0u;
            uint32_t m_passUpsample = 0u;
            Constants m_constants{};
    };
}