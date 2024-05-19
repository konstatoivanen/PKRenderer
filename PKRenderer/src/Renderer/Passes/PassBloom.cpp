#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Assets/AssetDatabase.h"
#include "Graphics/RHI/RHITexture.h"
#include "Graphics/RHI/RHICommandBuffer.h"
#include "Graphics/CommandBufferExt.h"
#include "Graphics/Shader.h"
#include "Renderer/HashCache.h"
#include "PassBloom.h"

namespace PK::Renderer::Passes
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::Graphics;
    using namespace PK::Graphics::RHI;

    PassBloom::PassBloom(AssetDatabase* assetDatabase, uint32_t initialWidth, uint32_t initialHeight)
    {
        PK_LOG_VERBOSE("PassBloom.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        TextureDescriptor descriptor{};
        descriptor.samplerType = SamplerType::Sampler2D;
        descriptor.usage = TextureUsage::DefaultStorage;
        descriptor.format = TextureFormat::RGB9E5;
        descriptor.formatAlias = TextureFormat::R32UI;
        descriptor.layers = 1;
        descriptor.levels = 8;
        descriptor.resolution = { initialWidth / 2u, initialHeight / 2u, 1u };
        descriptor.sampler.filterMin = FilterMode::Trilinear;
        descriptor.sampler.filterMag = FilterMode::Trilinear;

        m_bloomTexture = RHICreateTexture(descriptor, "Bloom.Texture");
        m_computeBloom = assetDatabase->Find<Shader>("CS_Bloom");
        m_passDownsample0 = m_computeBloom->GetRHIIndex("PASS_DOWNSAMPLE0");
        m_passDownsample = m_computeBloom->GetRHIIndex("PASS_DOWNSAMPLE1");
        m_passUpsample = m_computeBloom->GetRHIIndex("PASS_UPSAMPLE");
        m_passSeparableBlur = m_computeBloom->GetRHIIndex("PASS_BLUR");
    }

    void PassBloom::Render(CommandBufferExt cmd, Texture* source)
    {
        cmd->BeginDebugScope("Bloom", PK_COLOR_MAGENTA);

        auto bloom = m_bloomTexture.get();

        auto res = source->GetResolution();
        res.x >>= 1u;
        res.y >>= 1u;

        bloom->Validate(res);

        auto hash = HashCache::Get();

        RHISetTexture(hash->pk_Texture, source, 0, 0);
        RHISetImage(hash->pk_Image, bloom, 0, 0);
        RHISetTexture(hash->pk_Bloom_Texture, bloom);

        cmd.Dispatch(m_computeBloom, m_passDownsample0, { res.x, res.y, 1u });

        for (auto i = 1u; i < 8u; ++i)
        {
            RHISetTexture(hash->pk_Texture, bloom, i - 1u, 0);
            RHISetImage(hash->pk_Image, bloom, i, 0);
            cmd.Dispatch(m_computeBloom, m_passDownsample, { res.x >> i, res.y >> i, 1u });
        }

        for (auto i = 6; i >= 0; --i)
        {
            RHISetTexture(hash->pk_Texture, bloom, i + 1u, 0u);
            RHISetImage(hash->pk_Image, bloom, i, 0u);
            cmd.Dispatch(m_computeBloom, m_passUpsample, { res.x >> i, res.y >> i, 1u });
        }

        cmd->EndDebugScope();
    }

    Texture* PassBloom::GetTexture() { return m_bloomTexture.get(); }
}