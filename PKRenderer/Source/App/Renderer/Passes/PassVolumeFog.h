#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Math/Math.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    class PassVolumeFog : public NoCopy
    {
        public:
            struct ViewResources
            {
                RHITextureRef volumeInject;
                RHITextureRef volumeInjectPrev;
                RHITextureRef volumeDensity;
                RHITextureRef volumeDensityPrev;
                RHITextureRef volumeScatter;
                RHITextureRef volumeExtinction;
            };

            PassVolumeFog(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void ComputeDensity(CommandBufferExt cmd, struct RenderPipelineContext* context);
            void Compute(CommandBufferExt cmd, RenderPipelineContext* context);
            void Render(CommandBufferExt cmd, RHITexture* destination);

        private:
            ShaderAsset* m_compute = nullptr;
            constexpr static const uint32_t PASS_CLEAR = 0u;
            constexpr static const uint32_t PASS_DENSITY = 1u;
            constexpr static const uint32_t PASS_INJECT = 2u;
            constexpr static const uint32_t PASS_INTEGRATE = 3u;
            constexpr static const uint32_t PASS_COMPOSITE = 4u;
    };
}
