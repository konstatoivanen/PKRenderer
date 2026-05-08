#pragma once
#include "App/Renderer/RenderView.h"

namespace PK { class AssetDatabase; }

namespace PK::App
{
    class PassAutoExposure : public NoCopy
    {
        public:
            struct ViewResources : public IRenderViewResource
            {
                RHIBufferRef histogram;
            };

            PassAutoExposure(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void Render(CommandBufferExt cmd, struct RenderPipelineContext* context);

        private:
            ShaderAsset* m_compute = nullptr;
            uint32_t m_passHistogramBins = 0u;
            uint32_t m_passHistogramAvg = 0u;
    };
}
