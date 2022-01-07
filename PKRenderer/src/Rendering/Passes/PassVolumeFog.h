#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Objects/Shader.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Core;
    using namespace PK::Rendering::Objects;

    class PassVolumeFog : public PK::Core::NoCopy
    {
        public:
            PassVolumeFog(AssetDatabase* assetDatabase, const ApplicationConfig* config);
            void Render(CommandBuffer* cmd, RenderTexture* destination, const uint3& resolution);
            void OnUpdateParameters(const ApplicationConfig* config);

        private:
            Ref<ConstantBuffer> m_volumeResources;
            Ref<Buffer> m_depthTiles;
            Ref<Texture> m_volumeInject;
            Ref<Texture> m_volumeScatter;
            Shader* m_computeInject = nullptr;
            Shader* m_computeScatter = nullptr;
            Shader* m_computeDepthTiles = nullptr;
            Shader* m_shaderComposite = nullptr;
    };
}