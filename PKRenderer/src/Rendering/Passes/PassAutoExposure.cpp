#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Rendering/RHI/Objects/Shader.h"
#include "Rendering/RHI/Objects/Texture.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "Rendering/HashCache.h"
#include "PassAutoExposure.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    PassAutoExposure::PassAutoExposure(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE("PassAutoExposure.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_compute = assetDatabase->Find<Shader>("CS_AutoExposure");
        m_histogram = Buffer::Create<uint>(257ull, BufferUsage::DefaultStorage, "Histogram");
        m_passHistogramBins = m_compute->GetVariantIndex("PASS_HISTOGRAM");
        m_passHistogramAvg = m_compute->GetVariantIndex("PASS_AVG");
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