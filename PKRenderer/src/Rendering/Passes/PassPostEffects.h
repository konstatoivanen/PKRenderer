#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "ECS/Tokens/ViewProjectionToken.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::Rendering::Passes
{
    class PassPostEffectsComposite : public PK::Utilities::NoCopy
    {
        public:
            PassPostEffectsComposite(Core::Services::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void Render(RHI::Objects::CommandBuffer* cmd, RHI::Objects::Texture* destination);
            void OnUpdateParameters(PK::Core::Services::AssetImportToken<PK::Core::ApplicationConfig>* token);

        private:
            RHI::Objects::Shader* m_computeComposite = nullptr;
            Rendering::Objects::ConstantBufferRef m_constantsPostProcess;
            RHI::Objects::Texture* m_bloomLensDirtTexture;
            RHI::Objects::Texture* m_colorgradingLut;
            uint32_t m_passIndex = 0u;
    };
}