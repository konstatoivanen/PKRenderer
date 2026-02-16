#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "Core/Rendering/TextureAsset.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderViewSettings.h"
#include "PassTemporalAntialiasing.h"

namespace PK::App
{
    PassTemporalAntialiasing::PassTemporalAntialiasing(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE_FUNC("");
        m_computeTAA = assetDatabase->Find<ShaderAsset>("CS_TemporalAntialiasing").get();
    }

    void PassTemporalAntialiasing::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.TemporalAntialiasingSettings;
        view->constants.Set<float>(hash->pk_TAA_Sharpness, settings.Sharpness);
        view->constants.Set<float>(hash->pk_TAA_BlendingStatic, settings.BlendingStatic);
        view->constants.Set<float>(hash->pk_TAA_BlendingMotion, settings.BlendingMotion);
        view->constants.Set<float>(hash->pk_TAA_MotionAmplification, settings.MotionAmplification);
    }

    void PassTemporalAntialiasing::Render(CommandBufferExt cmd, RenderView* view, RHITexture* source, RHITexture* destination)
    {
        cmd->BeginDebugScope("TemporalAntialiasing", PK_COLOR_MAGENTA);

        auto hash = HashCache::Get();
        auto resources = view->GetResources<ViewResources>();

        uint16_t historyRead = m_historyLayerIndex++;
        m_historyLayerIndex %= 2;
        uint16_t historyWrite = m_historyLayerIndex;

        auto resolution = source->GetResolution();
        resolution.x *= 2;
        resolution.y *= 2;

        TextureDescriptor descriptor{};
        descriptor.format = TextureFormat::RGB9E5;
        descriptor.formatAlias = TextureFormat::R32UI;
        descriptor.resolution = resolution;
        descriptor.layers = 2;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;
        descriptor.sampler.wrap[0] = WrapMode::Clamp;
        descriptor.sampler.wrap[1] = WrapMode::Clamp;
        descriptor.sampler.wrap[2] = WrapMode::Clamp;
        descriptor.usage = TextureUsage::Default | TextureUsage::Storage;
        RHI::ValidateTexture(resources->historyTexture, descriptor, "TAA.HistoryTexture");

        RHI::SetTexture(hash->pk_Texture, source, { 0, 0, 1u, 1u });
        RHI::SetTexture(hash->pk_Texture1, resources->historyTexture.get(), { 0, historyRead, 1u, 1u });
        RHI::SetImage(hash->pk_Image, resources->historyTexture.get(), { 0, historyWrite, 1u, 1u });
        RHI::SetImage(hash->pk_Image1, destination, { 0, 0, 1u, 1u });
        cmd.Dispatch(m_computeTAA, 0, { resolution.x, resolution.y, 1u });
        cmd->EndDebugScope();

        m_jitter.z = m_jitter.x;
        m_jitter.w = m_jitter.y;
        m_jitter.x = Math::GetHaltonSequence((m_jitterSampleIndex & 1023) + 1, 2) - 0.5f;
        m_jitter.y = Math::GetHaltonSequence((m_jitterSampleIndex & 1023) + 1, 3) - 0.5f;
        m_jitter.x *= m_jitterSpread;
        m_jitter.y *= m_jitterSpread;
        m_jitterSampleIndex = (m_jitterSampleIndex + 1) % JITTER_SAMPLE_COUNT;
    }
}
