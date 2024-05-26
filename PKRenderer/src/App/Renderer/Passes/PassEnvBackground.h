#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    struct ApplicationConfig;

    class PassEnvBackground : public NoCopy
    {
        public:
            PassEnvBackground(AssetDatabase* assetDatabase);
            void ComputeSH(CommandBufferExt cmd);
            void RenderBackground(CommandBufferExt cmd);
            void OnUpdateParameters(AssetImportEvent<ApplicationConfig>* token);

        private:
            RHIBufferRef m_shBuffer;
            ShaderAsset* m_backgroundShader = nullptr;
            ShaderAsset* m_integrateSHShader = nullptr;
    };
}