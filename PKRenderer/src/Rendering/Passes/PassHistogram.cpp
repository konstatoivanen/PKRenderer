#include "PrecompiledHeader.h"
#include "PassHistogram.h"
#include "Rendering/HashCache.h"
#include "Rendering/GraphicsAPI.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Core;
    using namespace PK::Rendering::Objects;

    PassHistogram::PassHistogram(AssetDatabase* assetDatabase)
    {
        m_computeHistogram = assetDatabase->Find<Shader>("CS_Histogram");
        m_histogram = Buffer::CreateStorage(BufferLayout({ { ElementType::Uint, "BIN" } }), 257);
        m_passHistogramBins = m_computeHistogram->GetVariantIndex(StringHashID::StringToID("PASS_HISTOGRAM"));
        m_passHistogramAvg = m_computeHistogram->GetVariantIndex(StringHashID::StringToID("PASS_AVG"));
    }

    void PassHistogram::Execute(Texture* target, MemoryAccessFlags nextAccess)
    {
        auto hash = HashCache::Get();
        auto cmd = GraphicsAPI::GetCommandBuffer();
        auto histogram = m_histogram.get();

        auto res = target->GetResolution();
        cmd->SetTexture(hash->_MainTex, target, 0, 0);
        cmd->SetBuffer(hash->pk_Histogram, histogram);

        cmd->Dispatch(m_computeHistogram, m_passHistogramBins, { (uint)glm::ceil(res.x / 16.0f), (uint)glm::ceil(res.y / 16.0f), 1u });
        cmd->Barrier(histogram, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::ComputeRead);
        cmd->Dispatch(m_computeHistogram, m_passHistogramAvg, { 1u, 1u, 1u });
        cmd->Barrier(histogram, MemoryAccessFlags::ComputeWrite, nextAccess);
    }
}