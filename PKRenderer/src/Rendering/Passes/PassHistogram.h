#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/Buffer.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/CommandBuffer.h"

namespace PK::Rendering::Passes
{
    class PassHistogram : public Utilities::NoCopy
    {
        public:
            PassHistogram(Core::Services::AssetDatabase* assetDatabase);
            void Render(Objects::CommandBuffer* cmd, Objects::Texture* target);

        private:
            Objects::Shader* m_computeHistogram = nullptr;
            Utilities::Ref<Objects::Buffer> m_histogram;
            uint32_t m_passHistogramBins = 0u;
            uint32_t m_passHistogramAvg = 0u;
    };
}