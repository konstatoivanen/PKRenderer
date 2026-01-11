#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderViewSettings.h"
#include "App/Renderer/RenderPipelineBase.h"
#include "PassAutoExposure.h"

namespace PK::App
{
    PassAutoExposure::PassAutoExposure(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE_FUNC("");
        m_compute = assetDatabase->Find<ShaderAsset>("CS_AutoExposure").get();
        m_passHistogramBins = m_compute->GetRHIIndex("PASS_HISTOGRAM");
        m_passHistogramAvg = m_compute->GetRHIIndex("PASS_AVG");
    }

    void PassAutoExposure::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.AutoExposureSettings;
        view->constants->Set<float>(hash->pk_AutoExposure_LogLumaRange, settings.LogLuminanceRange);
        view->constants->Set<float>(hash->pk_AutoExposure_Min, settings.ExposureMin);
        view->constants->Set<float>(hash->pk_AutoExposure_Max, settings.ExposureMax);
        view->constants->Set<float>(hash->pk_AutoExposure_Target, settings.ExposureTarget);
        view->constants->Set<float>(hash->pk_AutoExposure_Speed, settings.ExposureSpeed);
    }

    void PassAutoExposure::Render(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto view = context->views[0];
        auto resources = view->GetResources<ViewResources>();
        auto target = view->GetGBuffersFullView().previous.color;
        auto resolution = target->GetResolution();

        if (RHI::ValidateBuffer<uint>(resources->histogram, 258ull, BufferUsage::DefaultStorage, "Histogram"))
        {
            RHI::SetBuffer(HashCache::Get()->pk_AutoExposure_Histogram, resources->histogram.get());
        }

        cmd->BeginDebugScope("AutoExposure", PK_COLOR_MAGENTA);

        RHI::SetTexture(HashCache::Get()->pk_Texture, target, 0, 0);

        cmd.Dispatch(m_compute, m_passHistogramBins, { resolution.x >> 1u, resolution.y >> 1u, 1u });
        cmd.Dispatch(m_compute, m_passHistogramAvg, { 1u, 1u, 1u });
        cmd->EndDebugScope();
    }
}
