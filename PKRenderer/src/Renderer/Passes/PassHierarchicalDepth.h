#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Graphics/GraphicsFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)

namespace PK::Renderer::Passes
{
    class PassHierarchicalDepth : public PK::Utilities::NoCopy
    {
        public:
            PassHierarchicalDepth(Core::Assets::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void Compute(Graphics::CommandBufferExt cmd, Math::uint3 resolution);

        private:
            Graphics::Shader* m_computeHierachicalDepth = nullptr;
            Graphics::TextureRef m_hierarchicalDepth;
    };
}