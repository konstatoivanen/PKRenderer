#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/Buffer.h"
#include "Rendering/Objects/Shader.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::Objects;

    class PassHistogram : public NoCopy
    {
        public:
            PassHistogram(AssetDatabase* assetDatabase);
            void Execute(Texture* target, MemoryAccessFlags nextAccess);

        private:
            Shader* m_computeHistogram = nullptr;
            Ref<Buffer> m_histogram;

            uint m_passHistogramBins = 0u;
            uint m_passHistogramAvg = 0u;
    };
}