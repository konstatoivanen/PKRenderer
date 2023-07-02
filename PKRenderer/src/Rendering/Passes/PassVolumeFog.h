#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Objects/Shader.h"

namespace PK::Rendering::Passes
{
    class PassVolumeFog : public PK::Utilities::NoCopy
    {
        public:
            PassVolumeFog(Core::Services::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void Compute(Objects::CommandBuffer* cmd, const Math::uint3& resolution);
            void Render(Objects::CommandBuffer* cmd, Objects::RenderTexture* destination);
            void OnUpdateParameters(const Core::ApplicationConfig* config);

        private:
            Utilities::Ref<Objects::ConstantBuffer> m_volumeResources;
            Utilities::Ref<Objects::Texture> m_volumeInject;
            Utilities::Ref<Objects::Texture> m_volumeInjectRead;
            Utilities::Ref<Objects::Texture> m_volumeScatter;
            Objects::Shader* m_computeInject = nullptr;
            Objects::Shader* m_computeScatter = nullptr;
            Objects::Shader* m_shaderComposite = nullptr;
    };
}