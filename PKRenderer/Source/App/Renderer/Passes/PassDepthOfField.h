#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    class PassDepthOfField : public NoCopy
    {
        public:
            struct ViewResources
            {
                RHITextureRef colorTarget;
                RHITextureRef alphaTarget;
                RHIBufferRef autoFocusBuffer;
            };

            PassDepthOfField(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void ComputeAutoFocus(CommandBufferExt cmd, struct RenderPipelineContext* context);
            void Render(CommandBufferExt cmd, RenderPipelineContext* context, RHITexture* destination);

        private:
            ShaderAsset* m_computeDepthOfField = nullptr;
            ShaderAsset* m_computeAutoFocus = nullptr;
            uint32_t m_passPrefilter = 0u;
            uint32_t m_passDiskblur = 0u;
            uint32_t m_passUpsample = 0u;
    };
}
