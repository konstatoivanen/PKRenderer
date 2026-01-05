#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Rendering/RenderingFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase);

namespace PK::App
{
    class PassBloom : public NoCopy
    {
        public:
            struct ViewResources
            {
                RHITextureRef bloomTexture;
            };

            PassBloom(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void Render(CommandBufferExt cmd, struct RenderPipelineContext* ctx);

        private:
            ShaderAsset* m_computeBloom = nullptr;
            //RHITextureRef m_bloomTexture;
            RHITexture* m_bloomLensDirtTexture;
            uint32_t m_passDownsample0 = 0;
            uint32_t m_passDownsample = 0;
            uint32_t m_passUpsample = 0;
    };
}
