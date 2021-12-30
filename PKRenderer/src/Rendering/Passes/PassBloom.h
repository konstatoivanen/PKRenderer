#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/Shader.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Core;
    using namespace PK::Utilities;
    using namespace PK::Rendering::Objects;

    class PassBloom : public NoCopy
    {
        public:
            PassBloom(AssetDatabase* assetDatabase, uint initialWidth, uint initialHeight);
            void Execute(RenderTexture* source, MemoryAccessFlags lastAccess);

            Texture* GetTexture() { return m_bloomTexture.get(); }

        private:
            Shader* m_computeBloom = nullptr;
            Ref<Texture> m_bloomTexture;
            uint m_passPrefilter = 0;
            uint m_passDiskblur = 0;
    };
}