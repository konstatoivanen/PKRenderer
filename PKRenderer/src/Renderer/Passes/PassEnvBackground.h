#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Graphics/GraphicsFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)

namespace PK::Renderer::Passes
{
    class PassEnvBackground : public PK::Utilities::NoCopy
    {
        public:
            PassEnvBackground(Core::Assets::AssetDatabase* assetDatabase);
            void ComputeSH(Graphics::CommandBufferExt cmd);
            void RenderBackground(Graphics::CommandBufferExt cmd);
            void OnUpdateParameters(PK::Core::Assets::AssetImportEvent<PK::Core::ApplicationConfig>* token);

        private:
            Graphics::BufferRef m_shBuffer;
            Graphics::Shader* m_backgroundShader = nullptr;
            Graphics::Shader* m_integrateSHShader = nullptr;
    };
}