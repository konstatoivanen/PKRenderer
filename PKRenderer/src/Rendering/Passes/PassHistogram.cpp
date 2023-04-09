#include "PrecompiledHeader.h"
#include "PassHistogram.h"
#include "Rendering/HashCache.h"
#include "Rendering/GraphicsAPI.h"

namespace PK::Rendering::Passes
{
    using namespace Core;
    using namespace Core::Services;
    using namespace Math;
    using namespace Objects;
    using namespace Structs;

    PassHistogram::PassHistogram(AssetDatabase* assetDatabase)
    {
        m_computeHistogram = assetDatabase->Find<Shader>("CS_Histogram");
        m_histogram = Buffer::Create(ElementType::Uint, 257, BufferUsage::DefaultStorage, "Histogram");
        m_passHistogramBins = m_computeHistogram->GetVariantIndex(StringHashID::StringToID("PASS_HISTOGRAM"));
        m_passHistogramAvg = m_computeHistogram->GetVariantIndex(StringHashID::StringToID("PASS_AVG"));
        GraphicsAPI::SetBuffer(HashCache::Get()->pk_Histogram, m_histogram.get());
    }

    void PassHistogram::Render(Objects::CommandBuffer* cmd, Texture* target)
    {
        cmd->BeginDebugScope("Histogram", PK_COLOR_MAGENTA);

        GraphicsAPI::SetTexture(HashCache::Get()->_MainTex, target, 0, 0);

        auto res = target->GetResolution();
        cmd->Dispatch(m_computeHistogram, m_passHistogramBins, { res.x, res.y, 1u });
        cmd->Dispatch(m_computeHistogram, m_passHistogramAvg, { 1u, 1u, 1u });
        cmd->EndDebugScope();
    }
}