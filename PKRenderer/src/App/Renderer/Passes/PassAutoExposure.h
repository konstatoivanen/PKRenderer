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
            PassAutoExposure(AssetDatabase* assetDatabase);
            void Render(CommandBufferExt cmd, RHITexture* target);

        private:
            ShaderAsset* m_compute = nullptr;
            RHIBufferRef m_histogram;
            uint32_t m_passHistogramBins = 0u;
            uint32_t m_passHistogramAvg = 0u;
    };
}