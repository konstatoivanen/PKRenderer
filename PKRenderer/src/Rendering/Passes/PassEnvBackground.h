#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Rendering/RHI/RHI.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)

namespace PK::Rendering::Passes
{
    class PassEnvBackground : public PK::Utilities::NoCopy
    {
        public:
            PassEnvBackground(Core::Assets::AssetDatabase* assetDatabase);
            void ComputeSH(RHI::Objects::CommandBuffer* cmd);
            void RenderBackground(RHI::Objects::CommandBuffer* cmd);
            void OnUpdateParameters(PK::Core::Assets::AssetImportEvent<PK::Core::ApplicationConfig>* token);

        private:
            RHI::Objects::BufferRef m_shBuffer;
            RHI::Objects::Shader* m_backgroundShader = nullptr;
            RHI::Objects::Shader* m_integrateSHShader = nullptr;
    };
}