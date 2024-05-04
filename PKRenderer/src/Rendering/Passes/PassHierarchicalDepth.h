#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Rendering/RHI/Objects/Texture.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, struct CommandBuffer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Shader)

namespace PK::Rendering::Passes
{
    class PassHierarchicalDepth : public PK::Utilities::NoCopy
    {
        public:
            PassHierarchicalDepth(Core::Assets::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void Compute(RHI::Objects::CommandBuffer* cmd, Math::uint3 resolution);

        private:
            RHI::Objects::Shader* m_computeHierachicalDepth = nullptr;
            RHI::Objects::TextureRef m_hierarchicalDepth;
    };
}