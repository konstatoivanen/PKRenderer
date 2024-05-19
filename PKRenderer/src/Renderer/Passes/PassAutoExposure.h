#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Graphics/GraphicsFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)

namespace PK::Renderer::Passes
{
    class PassAutoExposure : public Utilities::NoCopy
    {
        public:
            PassAutoExposure(Core::Assets::AssetDatabase* assetDatabase);
            void Render(Graphics::CommandBufferExt cmd, Graphics::Texture* target);

        private:
            Graphics::Shader* m_compute = nullptr;
            Graphics::BufferRef m_histogram;
            uint32_t m_passHistogramBins = 0u;
            uint32_t m_passHistogramAvg = 0u;
    };
}