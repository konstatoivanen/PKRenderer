#include "PrecompiledHeader.h"
#include "PassTemporalAntiAliasing.h"
#include "Rendering/HashCache.h"
#include "Rendering/GraphicsAPI.h"
#include "Math/FunctionsMisc.h"

namespace PK::Rendering::Passes
{
    using namespace Core::Services;
    using namespace Math;
    using namespace Objects;
    using namespace Structs;

    PassTemporalAntialiasing::PassTemporalAntialiasing(AssetDatabase* assetDatabase, uint32_t initialWidth, uint32_t initialHeight)
    {
        m_computeTAA = assetDatabase->Find<Shader>("CS_TemporalAntialiasing");

        TextureDescriptor descriptor{};
        descriptor.format = TextureFormat::RGB9E5;
        descriptor.resolution.x = initialWidth * 2;
        descriptor.resolution.y = initialHeight * 2;
        descriptor.layers = 2;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;
        descriptor.sampler.wrap[0] = WrapMode::Clamp;
        descriptor.sampler.wrap[1] = WrapMode::Clamp;
        descriptor.sampler.wrap[2] = WrapMode::Clamp;
        descriptor.usage = TextureUsage::Default | TextureUsage::Storage | TextureUsage::Aliased;
        m_renderTarget = Texture::Create(descriptor, "TAA.HistoryTexture");
    }

    void PassTemporalAntialiasing::Render(CommandBuffer* cmd, RenderTexture* source)
    {
        cmd->BeginDebugScope("TemporalAntialiasing", PK_COLOR_MAGENTA);

        auto hash = HashCache::Get();

        uint16_t historyRead = m_historyLayerIndex++;
        m_historyLayerIndex %= 2;
        uint16_t historyWrite = m_historyLayerIndex;

        auto resolution = source->GetResolution();
        resolution.x *= 2;
        resolution.y *= 2;

        m_renderTarget->Validate(resolution);

        GraphicsAPI::SetTexture(hash->_SourceTex, source->GetColor(0u), { 0, 0, 1u, 1u });
        GraphicsAPI::SetTexture(hash->_HistoryReadTex, m_renderTarget.get(), { 0, historyRead, 1u, 1u });
        GraphicsAPI::SetImage(hash->_HistoryWriteTex, m_renderTarget.get(), { 0, historyWrite, 1u, 1u });
        cmd->Dispatch(m_computeTAA, 0, { resolution.x, resolution.y, 1u });
        cmd->Blit(m_renderTarget.get(), source->GetColor(0), { 0, historyWrite, 1u, 1u }, { 0, 0, 1u, 1u }, FilterMode::Bilinear);
        cmd->EndDebugScope();

        m_jitter.z = m_jitter.x;
        m_jitter.w = m_jitter.y;
        m_jitter.x = Functions::GetHaltonSequence((m_jitterSampleIndex & 1023) + 1, 2) - 0.5f;
        m_jitter.y = Functions::GetHaltonSequence((m_jitterSampleIndex & 1023) + 1, 3) - 0.5f;
        m_jitter.x *= m_jitterSpread;
        m_jitter.y *= m_jitterSpread;
        m_jitterSampleIndex = (m_jitterSampleIndex + 1) % JitterSampleCount;
    }
}