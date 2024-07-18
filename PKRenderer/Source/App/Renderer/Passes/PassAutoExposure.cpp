#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderViewSettings.h"
#include "PassAutoExposure.h"

namespace PK::App
{
    PassAutoExposure::PassAutoExposure(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE("PassAutoExposure.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_compute = assetDatabase->Find<ShaderAsset>("CS_AutoExposure");
        m_histogram = RHI::CreateBuffer<uint>(257ull, BufferUsage::DefaultStorage, "Histogram");
        m_passHistogramBins = m_compute->GetRHIIndex("PASS_HISTOGRAM");
        m_passHistogramAvg = m_compute->GetRHIIndex("PASS_AVG");
        RHI::SetBuffer(HashCache::Get()->pk_AutoExposure_Histogram, m_histogram.get());
    }

    void PassAutoExposure::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settingsRef->AutoExposureSettings;
        view->constants->Set<float>(hash->pk_AutoExposure_MinLogLuma, settings.LuminanceMin);
        view->constants->Set<float>(hash->pk_AutoExposure_InvLogLumaRange, 1.0f / settings.LuminanceRange);
        view->constants->Set<float>(hash->pk_AutoExposure_LogLumaRange, settings.LuminanceRange);
        view->constants->Set<float>(hash->pk_AutoExposure_Target, settings.ExposureTarget);
        view->constants->Set<float>(hash->pk_AutoExposure_Speed, settings.ExposureSpeed);
    }

    void PassAutoExposure::Render(CommandBufferExt cmd, RHITexture* target)
    {
        cmd->BeginDebugScope("AutoExposure", PK_COLOR_MAGENTA);

        RHI::SetTexture(HashCache::Get()->pk_Texture, target, 0, 0);

        auto res = target->GetResolution();
        cmd.Dispatch(m_compute, m_passHistogramBins, { res.x, res.y, 1u });
        cmd.Dispatch(m_compute, m_passHistogramAvg, { 1u, 1u, 1u });
        cmd->EndDebugScope();
    }
}