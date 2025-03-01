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
            ShaderAsset* m_computeDensity = nullptr;
            ShaderAsset* m_computeInject = nullptr;
            ShaderAsset* m_computeScatter = nullptr;
            ShaderAsset* m_shaderComposite = nullptr;
    };
}