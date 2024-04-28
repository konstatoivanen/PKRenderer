#pragma once
#include "Utilities/ForwardDeclareUtility.h"
#include "Utilities/NoCopy.h"
#include "Core/Assets/AssetImportEvent.h"
#include "Rendering/RHI/Objects/Buffer.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, struct CommandBuffer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Shader)

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