#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Rendering/RenderingFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)

namespace PK::App
{
    class PassAutoExposure : public NoCopy
    {
        public:
            struct ViewResources
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
