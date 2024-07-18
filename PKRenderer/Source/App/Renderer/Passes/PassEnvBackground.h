#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    class PassEnvBackground : public NoCopy
    {
        public:
            PassEnvBackground(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void ComputeSH(CommandBufferExt cmd);
            void RenderBackground(CommandBufferExt cmd);

        private:
            RHITexture* m_backgroundTexture = nullptr;
            ShaderAsset* m_backgroundShader = nullptr;
            ShaderAsset* m_integrateSHShader = nullptr;
            RHIBufferRef m_shBuffer;
    };
}