#pragma once
#include "Utilities/ForwardDeclareUtility.h"
#include "Utilities/NoCopy.h"
#include "Rendering/RHI/Objects/Texture.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, struct CommandBuffer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Shader)

namespace PK::Rendering::Passes
{
    class PassFilmGrain : public PK::Utilities::NoCopy
    {
        public:
            PassFilmGrain(Core::Assets::AssetDatabase* assetDatabase);
            void Compute(RHI::Objects::CommandBuffer* cmd);

        private:
            RHI::Objects::Shader* m_computeFilmGrain = nullptr;
            RHI::Objects::TextureRef m_filmGrainTexture;
    };
}