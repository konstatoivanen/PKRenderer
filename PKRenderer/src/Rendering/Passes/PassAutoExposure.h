#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Rendering/RHI/RHI.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)

namespace PK::Rendering::Passes
{
    class PassAutoExposure : public Utilities::NoCopy
    {
        public:
            PassAutoExposure(Core::Assets::AssetDatabase* assetDatabase);
            void Render(RHI::Objects::CommandBuffer* cmd, RHI::Objects::Texture* target);

        private:
            RHI::Objects::Shader* m_compute = nullptr;
            RHI::Objects::BufferRef m_histogram;
            uint32_t m_passHistogramBins = 0u;
            uint32_t m_passHistogramAvg = 0u;
    };
}