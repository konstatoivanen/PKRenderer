#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Core/Rendering/RenderingFwd.h"
#include "Core/Math/Math.h"

namespace PK::App
{
    class PassSceneEnv : public NoCopy
    {
        public:
            struct ViewResources
            {
                RHITexture* sourceTexture = nullptr;
                RHITextureRef sceneEnvTexture = nullptr;
                RHIBufferRef sceneEnvSHBuffer = nullptr;
                int32_t captureCounter = 0;
                float3 captureOrigin = PK_FLOAT3_ZERO;
                bool captureIsDirty = false;
            };

            PassSceneEnv(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void PreCompute(CommandBufferExt cmd, struct RenderPipelineContext* context);
            void RenderBackground(CommandBufferExt cmd, RenderPipelineContext* context);

        private:
            ShaderAsset* m_backgroundShader = nullptr;
            ShaderAsset* m_shShader = nullptr;
            ShaderAsset* m_integrateSHShader = nullptr;
            bool m_forceCapture = false;
    };
}
