#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/RHI/RHI.h"

namespace PK::Rendering::Passes
{
    class PassVolumeFog : public PK::Utilities::NoCopy
    {
        public:
            PassVolumeFog(Core::Assets::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void ComputeDensity(RHI::Objects::CommandBuffer* cmd, const Math::uint3& resolution);
            void Compute(RHI::Objects::CommandBuffer* cmd, const Math::uint3& resolution);
            void Render(RHI::Objects::CommandBuffer* cmd, RHI::Objects::Texture* destination);
            void OnUpdateParameters(const Core::ApplicationConfig* config);

        private:
            Rendering::Objects::ConstantBufferRef m_volumeResources;
            RHI::Objects::TextureRef m_volumeInject;
            RHI::Objects::TextureRef m_volumeInjectPrev;
            RHI::Objects::TextureRef m_volumeDensity;
            RHI::Objects::TextureRef m_volumeDensityPrev;
            RHI::Objects::TextureRef m_volumeScatter;
            RHI::Objects::TextureRef m_volumeExtinction;
            RHI::Objects::Shader* m_computeDensity = nullptr;
            RHI::Objects::Shader* m_computeInject = nullptr;
            RHI::Objects::Shader* m_computeScatter = nullptr;
            RHI::Objects::Shader* m_shaderComposite = nullptr;
    };
}