#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::Rendering::Passes
{
    class PassEnvBackground : public PK::Utilities::NoCopy
    {
        public:
            PassEnvBackground(Core::Services::AssetDatabase* assetDatabase);
            void ComputeSH(RHI::Objects::CommandBuffer* cmd);
            void RenderBackground(RHI::Objects::CommandBuffer* cmd);
            void OnUpdateParameters(PK::Core::Services::AssetImportToken<PK::Core::ApplicationConfig>* token);

        private:
            RHI::Objects::BufferRef m_shBuffer;
            RHI::Objects::Shader* m_backgroundShader = nullptr;
            RHI::Objects::Shader* m_integrateSHShader = nullptr;
    };
}