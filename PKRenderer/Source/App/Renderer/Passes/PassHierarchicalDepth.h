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
            PassHierarchicalDepth(AssetDatabase* assetDatabase, const uint2& initialResolution);
            void Compute(CommandBufferExt cmd, uint3 resolution);

        private:
            ShaderAsset* m_computeHierachicalDepth = nullptr;
            RHITextureRef m_hierarchicalDepth;
    };
}