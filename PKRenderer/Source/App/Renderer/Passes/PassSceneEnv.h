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
                RHITextureRef sceneEnvIBL = nullptr;
                RHITextureRef sceneEnvISL = nullptr;
                RHIBufferRef sceneEnvSHBuffer = nullptr;
                float prevExposure = 0.0f;
                float prevDensity = 0.0f;
                float4 prevFogExpParams0 = PK_FLOAT4_ZERO;
                float4 prevFogExpParams1 = PK_FLOAT4_ZERO;
                int32_t captureCounter = 0u;
                float3 captureOrigin = PK_FLOAT3_ZERO;
                bool captureIsDirty = false;
            };

            PassSceneEnv(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void PreCompute(CommandBufferExt cmd, struct RenderPipelineContext* context);
            void RenderBackground(CommandBufferExt cmd, RenderPipelineContext* context);

        private:
            ShaderAsset* m_backgroundShader = nullptr;
            ShaderAsset* m_integrateSHShader = nullptr;
            ShaderAsset* m_integrateIBLShader = nullptr;
            ShaderAsset* m_integrateISLShader = nullptr;
            bool m_forceCapture = false;
    };
}
