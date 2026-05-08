#pragma once
#include "App/Renderer/RenderView.h"

namespace PK { class AssetDatabase; }

namespace PK::App
{
    class PassDistort : public NoCopy
    {
    public:
        struct ViewResources : public IRenderViewResource
        {
            RHITextureRef distortTexture;
        };

        PassDistort(AssetDatabase* assetDatabase);
        void SetViewConstants(struct RenderView* view);
        void Render(CommandBufferExt cmd, struct RenderPipelineContext* ctx);

    private:
        ShaderAsset* m_computeDistort = nullptr;
    };
}
