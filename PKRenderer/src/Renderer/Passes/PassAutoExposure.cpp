#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Graphics/RHI/RHITexture.h"
#include "Graphics/RHI/RHIBuffer.h"
#include "Graphics/RHI/RHICommandBuffer.h"
#include "Graphics/CommandBufferExt.h"
#include "Graphics/Shader.h"
#include "Renderer/HashCache.h"
#include "PassAutoExposure.h"

namespace PK::Renderer::Passes
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::Graphics;
    using namespace PK::Graphics::RHI;

    PassAutoExposure::PassAutoExposure(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE("PassAutoExposure.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_compute = assetDatabase->Find<Shader>("CS_AutoExposure");
        m_histogram = RHICreateBuffer<uint>(257ull, BufferUsage::DefaultStorage, "Histogram");
        m_passHistogramBins = m_compute->GetRHIIndex("PASS_HISTOGRAM");
        m_passHistogramAvg = m_compute->GetRHIIndex("PASS_AVG");
        RHISetBuffer(HashCache::Get()->pk_AutoExposure_Histogram, m_histogram.get());
    }

    void PassAutoExposure::Render(CommandBufferExt cmd, Texture* target)
    {
        cmd->BeginDebugScope("AutoExposure", PK_COLOR_MAGENTA);

        RHISetTexture(HashCache::Get()->pk_Texture, target, 0, 0);

        auto res = target->GetResolution();
        cmd.Dispatch(m_compute, m_passHistogramBins, { res.x, res.y, 1u });
        cmd.Dispatch(m_compute, m_passHistogramAvg, { 1u, 1u, 1u });
        cmd->EndDebugScope();
    }
}