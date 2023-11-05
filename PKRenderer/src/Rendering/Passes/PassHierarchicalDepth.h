#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::Rendering::Passes
{
    class PassHierarchicalDepth : public PK::Utilities::NoCopy
    {
        public:
            PassHierarchicalDepth(Core::Services::AssetDatabase* assetDatabase, Core::ApplicationConfig* config);
            void Compute(RHI::Objects::CommandBuffer* cmd, Math::uint3 resolution);

        private:
            RHI::Objects::Shader* m_computeHierachicalDepth = nullptr;
            RHI::Objects::TextureRef m_hierarchicalDepth;
    };
}