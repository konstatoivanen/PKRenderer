#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Math/Math.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    class PassVolumeFog : public NoCopy
    {
        public:
            PassVolumeFog(AssetDatabase* assetDatabase, const uint2& initialResolution);
            void SetViewConstants(struct RenderView* view);
            void ComputeDensity(CommandBufferExt cmd, const uint3& resolution);
            void Compute(CommandBufferExt cmd, const uint3& resolution);
            void Render(CommandBufferExt cmd, RHITexture* destination);

        private:
            ShaderAsset* m_computeDensity = nullptr;
            ShaderAsset* m_computeInject = nullptr;
            ShaderAsset* m_computeScatter = nullptr;
            ShaderAsset* m_shaderComposite = nullptr;
            RHITextureRef m_volumeInject;
            RHITextureRef m_volumeInjectPrev;
            RHITextureRef m_volumeDensity;
            RHITextureRef m_volumeDensityPrev;
            RHITextureRef m_volumeScatter;
            RHITextureRef m_volumeExtinction;
    };
}