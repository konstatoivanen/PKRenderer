#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    class PassVolumeFog : public NoCopy
    {
        public:
            PassVolumeFog(AssetDatabase* assetDatabase, const ApplicationConfig* config);
            void ComputeDensity(CommandBufferExt cmd, const uint3& resolution);
            void Compute(CommandBufferExt cmd, const uint3& resolution);
            void Render(CommandBufferExt cmd, RHITexture* destination);
            void OnUpdateParameters(const ApplicationConfig* config);

        private:
            ShaderAsset* m_computeDensity = nullptr;
            ShaderAsset* m_computeInject = nullptr;
            ShaderAsset* m_computeScatter = nullptr;
            ShaderAsset* m_shaderComposite = nullptr;
            ConstantBufferRef m_volumeResources;
            RHITextureRef m_volumeInject;
            RHITextureRef m_volumeInjectPrev;
            RHITextureRef m_volumeDensity;
            RHITextureRef m_volumeDensityPrev;
            RHITextureRef m_volumeScatter;
            RHITextureRef m_volumeExtinction;
    };
}