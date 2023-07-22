#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/CommandBuffer.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Texture.h"

namespace PK::Rendering::Passes
{
    class PassHierarchicalDepth : public PK::Utilities::NoCopy
    {
        public:
            PassHierarchicalDepth(Core::Services::AssetDatabase* assetDatabase, Core::ApplicationConfig* config);
            void Compute(Objects::CommandBuffer* cmd, Math::uint3 resolution);

        private:
            Objects::Shader* m_computeHierachicalDepth = nullptr;
            Utilities::Ref<Objects::Texture> m_hierarchicalDepth;
    };
}