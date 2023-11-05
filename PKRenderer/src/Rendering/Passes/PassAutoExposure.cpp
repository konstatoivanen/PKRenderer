#include "PrecompiledHeader.h"
#include "Rendering/HashCache.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "PassAutoExposure.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::Services;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    PassAutoExposure::PassAutoExposure(AssetDatabase* assetDatabase)
    {
        m_compute = assetDatabase->Find<Shader>("CS_AutoExposure");
        m_histogram = Buffer::Create(ElementType::Uint, 257, BufferUsage::DefaultStorage, "Histogram");
        m_passHistogramBins = m_compute->GetVariantIndex(StringHashID::StringToID("PASS_HISTOGRAM"));
        m_passHistogramAvg = m_compute->GetVariantIndex(StringHashID::StringToID("PASS_AVG"));
        GraphicsAPI::SetBuffer(HashCache::Get()->pk_AutoExposure_Histogram, m_histogram.get());
    }

    void PassAutoExposure::Render(CommandBuffer* cmd, Texture* target)
    {
        cmd->BeginDebugScope("AutoExposure", PK_COLOR_MAGENTA);

        GraphicsAPI::SetTexture(HashCache::Get()->pk_Texture, target, 0, 0);

        auto res = target->GetResolution();
        cmd->Dispatch(m_compute, m_passHistogramBins, { res.x, res.y, 1u });
        cmd->Dispatch(m_compute, m_passHistogramAvg, { 1u, 1u, 1u });
        cmd->EndDebugScope();
    }
}