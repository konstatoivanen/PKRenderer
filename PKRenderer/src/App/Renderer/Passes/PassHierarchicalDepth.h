#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    struct ApplicationConfig;

    class PassHierarchicalDepth : public NoCopy
    {
        public:
            PassHierarchicalDepth(AssetDatabase* assetDatabase, const ApplicationConfig* config);
            void Compute(CommandBufferExt cmd, uint3 resolution);

        private:
            ShaderAsset* m_computeHierachicalDepth = nullptr;
            RHITextureRef m_hierarchicalDepth;
    };
}