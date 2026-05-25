#pragma once
#include "App/Renderer/RenderView.h"

namespace PK { class AssetDatabase; }

namespace PK::App
{
    class PassBloom : public NoCopy
    {
        public:
            struct ViewResources : public IRenderViewResource
            {
                RHITextureRef bloomTexture;
            };

            PassBloom(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void Render(CommandBufferExt cmd, struct RenderPipelineContext* ctx);

        private:
            ShaderAsset* m_computeBloom;
            RHITexture* m_bloomLensDirtTexture;
            uint32_t m_passDownsample0;
            uint32_t m_passDownsample;
            uint32_t m_passUpsample;
    };
}
