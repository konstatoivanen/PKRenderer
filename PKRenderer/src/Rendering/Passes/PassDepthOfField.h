#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Objects/Shader.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Core;
    using namespace PK::Rendering::Objects;

    class PassDepthOfField : public PK::Core::NoCopy
    {
        struct Constants
        {
            float pk_FocalLength;
            float pk_FNumber;
            float pk_FilmHeight;
            float pk_FocusSpeed;
            float pk_MaximumCoC;
        };

        public:
            PassDepthOfField(AssetDatabase* assetDatabase, const ApplicationConfig* config);
            void Execute(RenderTexture* destination, MemoryAccessFlags lastAccess);
            void OnUpdateParameters(const ApplicationConfig* config);

        private:
            Shader* m_shaderBlur = nullptr;
            Shader* m_shaderComposite = nullptr;
            Shader* m_computeAutoFocus = nullptr;
            Ref<Texture> m_renderTarget;
            Ref<Buffer> m_autoFocusParams;
            uint m_passPrefilter = 0u;
            uint m_passDiskblur = 0u;

            Constants m_constants{};
    };
}