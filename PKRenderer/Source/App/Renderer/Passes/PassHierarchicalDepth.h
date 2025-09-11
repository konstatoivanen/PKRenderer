#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Rendering/RenderingFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)

namespace PK::App
{
    class PassHierarchicalDepth : public NoCopy
    {
        public:
            struct ViewResources
            {
                RHITextureRef hierarchicalDepth;
            };

            PassHierarchicalDepth(AssetDatabase* assetDatabase);
            void Compute(CommandBufferExt cmd, struct RenderPipelineContext* context);

        private:
            ShaderAsset* m_computeHierachicalDepth = nullptr;
    };
}
