#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK { class AssetDatabase; }

namespace PK::App
{
    class PassDistort : public NoCopy
    {
    public:
        struct ViewResources
        {
            RHITextureRef distortTexture;
        };

        PassDistort(AssetDatabase* assetDatabase);
        void SetViewConstants(struct RenderView* view);
        void Render(CommandBufferExt cmd, struct RenderPipelineContext* ctx);

    private:
        ShaderAsset* m_computeDistort = nullptr;
    };
}
