#pragma once
#include "App/Renderer/RenderView.h"

namespace PK { class AssetDatabase; }

namespace PK::App
{
    struct RenderView;

    class PassHierarchicalDepth : public NoCopy
    {
        public:
            struct ViewResources : public IRenderViewResource
            {
                RHITextureRef hierarchicalDepth;
            };

            PassHierarchicalDepth(AssetDatabase* assetDatabase);
            void SetViewConstants(RenderView* view);
            void Compute(CommandBufferExt cmd, struct RenderPipelineContext* context);

        private:
            ShaderAsset* m_computeHierachicalDepth = nullptr;
            RHIBufferRef m_worgroupCounter;
    };
}
