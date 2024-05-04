#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Rendering/RHI/Objects/Buffer.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, struct CommandBuffer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Shader)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Texture)

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