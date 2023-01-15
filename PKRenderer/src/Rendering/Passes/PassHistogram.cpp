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
        m_histogram = Buffer::CreateStorage(BufferLayout({ { ElementType::Uint, "BIN" } }), 257, BufferUsage::None, "Histogram");
        m_passHistogramBins = m_computeHistogram->GetVariantIndex(StringHashID::StringToID("PASS_HISTOGRAM"));
        m_passHistogramAvg = m_computeHistogram->GetVariantIndex(StringHashID::StringToID("PASS_AVG"));
    }

    void PassHistogram::Execute(Objects::CommandBuffer* cmd, Texture* target, MemoryAccessFlags nextAccess)
    {
        cmd->BeginDebugScope("Histogram", PK_COLOR_MAGENTA);

        auto hash = HashCache::Get();
        auto histogram = m_histogram.get();

        auto res = target->GetResolution();
        cmd->SetTexture(hash->_MainTex, target, 0, 0);
        cmd->SetBuffer(hash->pk_Histogram, histogram);

        cmd->Dispatch(m_computeHistogram, m_passHistogramBins, { (uint)glm::ceil(res.x / 16.0f), (uint)glm::ceil(res.y / 16.0f), 1u });
        cmd->Barrier(histogram, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::ComputeRead);
        cmd->Dispatch(m_computeHistogram, m_passHistogramAvg, { 1u, 1u, 1u });
        cmd->Barrier(histogram, MemoryAccessFlags::ComputeWrite, nextAccess);
    
        cmd->EndDebugScope();
    }
}