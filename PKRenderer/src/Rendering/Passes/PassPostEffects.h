#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "ECS/Tokens/ViewProjectionToken.h"

namespace PK::Rendering::Passes
{
    class PassPostEffectsComposite : public PK::Utilities::NoCopy
    {
        public:
            PassPostEffectsComposite(Core::Services::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void Render(Objects::CommandBuffer* cmd, Objects::Texture* destination);
            void OnUpdateParameters(PK::Core::Services::AssetImportToken<PK::Core::ApplicationConfig>* token);

        private:
            Objects::Shader* m_computeComposite = nullptr;
            Utilities::Ref<Objects::ConstantBuffer> m_constantsPostProcess;
            Objects::Texture* m_bloomLensDirtTexture;
            Objects::Texture* m_lut;
    };
}