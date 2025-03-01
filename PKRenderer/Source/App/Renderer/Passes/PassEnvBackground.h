#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    class PassEnvBackground : public NoCopy
    {
        public:
            struct ViewResources
            {
                RHITexture* sourceTexture = nullptr;
                RHITextureRef sceneEnvTexture = nullptr;
                RHIBufferRef sceneEnvSHBuffer = nullptr;
                bool runtimeIsDirty = false;
            };

            PassEnvBackground(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void PreCompute(CommandBufferExt cmd, struct RenderPipelineContext* context);
            void RenderBackground(CommandBufferExt cmd, RenderPipelineContext* context);

        private:
            ShaderAsset* m_backgroundShader = nullptr;
            ShaderAsset* m_shShader = nullptr;
            ShaderAsset* m_integrateSHShader = nullptr;
    };
}
