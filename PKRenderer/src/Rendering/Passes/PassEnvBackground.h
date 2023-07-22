#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/CommandBuffer.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Texture.h"

namespace PK::Rendering::Passes
{
    class PassEnvBackground : public PK::Utilities::NoCopy
    {
        public:
            PassEnvBackground(Core::Services::AssetDatabase* assetDatabase);
            void ComputeSH(Objects::CommandBuffer* cmd);
            void RenderBackground(Objects::CommandBuffer* cmd);
            void OnUpdateParameters(PK::Core::Services::AssetImportToken<PK::Core::ApplicationConfig>* token);

        private:
            Utilities::Ref<Objects::Buffer> m_shBuffer;
            Objects::Shader* m_backgroundShader = nullptr;
            Objects::Shader* m_integrateSHShader = nullptr;
    };
}