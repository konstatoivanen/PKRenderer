#pragma once
#include "Utilities/NoCopy.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Renderer::Passes
{
    class PassVolumeFog : public PK::Utilities::NoCopy
    {
        public:
            PassVolumeFog(Core::Assets::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void ComputeDensity(Graphics::CommandBufferExt cmd, const Math::uint3& resolution);
            void Compute(Graphics::CommandBufferExt cmd, const Math::uint3& resolution);
            void Render(Graphics::CommandBufferExt cmd, Graphics::Texture* destination);
            void OnUpdateParameters(const Core::ApplicationConfig* config);

        private:
            Graphics::ConstantBufferRef m_volumeResources;
            Graphics::TextureRef m_volumeInject;
            Graphics::TextureRef m_volumeInjectPrev;
            Graphics::TextureRef m_volumeDensity;
            Graphics::TextureRef m_volumeDensityPrev;
            Graphics::TextureRef m_volumeScatter;
            Graphics::TextureRef m_volumeExtinction;
            Graphics::Shader* m_computeDensity = nullptr;
            Graphics::Shader* m_computeInject = nullptr;
            Graphics::Shader* m_computeScatter = nullptr;
            Graphics::Shader* m_shaderComposite = nullptr;
    };
}