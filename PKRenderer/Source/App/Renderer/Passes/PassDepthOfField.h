#pragma once
#include "App/Renderer/RenderView.h"

namespace PK::App
{
    class PassDepthOfField : public NoCopy
    {
        public:
            struct ViewResources : public IRenderViewResource
            {
                RHITextureRef colorTarget;
                RHITextureRef alphaTarget;
                RHIBufferRef autoFocusBuffer;
            };

            PassDepthOfField(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void ComputeAutoFocus(RenderGraphExt graph, QueueType queue, struct RenderPipelineContext* context);
            void Render(RenderGraphExt graph, QueueType queue, RenderPipelineContext* context, RHITexture* destination);

        private:
            ShaderAsset* m_computeDepthOfField = nullptr;
            ShaderAsset* m_computeAutoFocus = nullptr;
            uint32_t m_passPrefilter = 0u;
            uint32_t m_passDiskblur = 0u;
            uint32_t m_passUpsample = 0u;
    };
}
