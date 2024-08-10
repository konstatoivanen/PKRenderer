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
            // @TODO use this to add dirty checks for directional light
            void SetDirty() { m_isDirty = true; }
            void PreCompute(CommandBufferExt cmd);
            void RenderBackground(CommandBufferExt cmd);

        private:
            ShaderAsset* m_backgroundShader = nullptr;
            ShaderAsset* m_shShader = nullptr;
            ShaderAsset* m_integrateShader = nullptr;
            
            RHITexture* m_sourceTexture = nullptr;
            RHITextureRef m_backgroundTexture = nullptr;
            RHIBufferRef m_shBuffer;
            bool m_isDirty = false;
    };
}
