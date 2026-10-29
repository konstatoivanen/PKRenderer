#include "PrecompiledHeader.h"
#include "Core/Math/Random.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/RenderGraphExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "Core/Rendering/TextureAsset.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "PassTemporalAntialiasing.h"

namespace PK::App
{
    PassTemporalAntialiasing::PassTemporalAntialiasing(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE_FUNC();
        m_computeTAA = assetDatabase->Find<ShaderAsset>("CS_TemporalAntialiasing").get();
    }

    void PassTemporalAntialiasing::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.TemporalAntialiasingSettings;
        view->constants.Set<float>(hash->TAA_Sharpness, settings.Sharpness);
        view->constants.Set<float>(hash->TAA_BlendingStatic, settings.BlendingStatic);
        view->constants.Set<float>(hash->TAA_BlendingMotion, settings.BlendingMotion);
        view->constants.Set<float>(hash->TAA_MotionAmplification, settings.MotionAmplification);
    }

    void PassTemporalAntialiasing::Render(RenderGraphExt graph, QueueType queue, RenderView* view, RHITexture* source, RHITexture* destination)
    {
        graph->BeginDebugScope("TemporalAntialiasing", PK_COLOR_MAGENTA);

        auto hash = HashCache::Get();
        auto resources = view->GetResource<ViewResources>();

        auto historyRead = (uint16_t)m_historyLayerIndex++;
        m_historyLayerIndex %= 2;
        auto historyWrite = (uint16_t)m_historyLayerIndex;

        auto resolution = source->GetResolution();
        resolution.x *= 2;
        resolution.y *= 2;

        TextureDescriptor descriptor{};
        descriptor.format = TextureFormat::RGB9E5_Float;
        descriptor.formatAlias = TextureFormat::R32_Uint;
        descriptor.resolution = resolution;
        descriptor.layers = 2;
        descriptor.usage = TextureUsage::Default | TextureUsage::Storage;
        RHI::ValidateTexture(resources->historyTexture, descriptor, "TAA.HistoryTexture");

        RHI::SetTexture(hash->pk_Texture, source, { 0, 0, 1u, 1u });
        RHI::SetTexture(hash->pk_Texture1, resources->historyTexture.get(), { 0, historyRead, 1u, 1u });
        RHI::SetImage(hash->pk_Image, resources->historyTexture.get(), { 0, historyWrite, 1u, 1u });
        RHI::SetImage(hash->pk_Image1, destination, { 0, 0, 1u, 1u });
        graph.Dispatch(queue, m_computeTAA, 0, { resolution.x, resolution.y, 1u });
        graph->EndDebugScope();

        m_jitter.zw = m_jitter.xy;
        m_jitter.xy = math::halton((m_jitterSampleIndex & 1023) + 1, uint2(2,3)) - 0.5f;
        m_jitter.xy *= m_jitterSpread;
        m_jitterSampleIndex = (m_jitterSampleIndex + 1) % JITTER_SAMPLE_COUNT;
    }
}
