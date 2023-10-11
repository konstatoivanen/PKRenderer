#include "PrecompiledHeader.h"
#include "PassBloom.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/HashCache.h"

namespace PK::Rendering::Passes
{
    using namespace Core::Services;
    using namespace Math;
    using namespace Objects;
    using namespace Structs;

    PassBloom::PassBloom(AssetDatabase* assetDatabase, uint32_t initialWidth, uint32_t initialHeight)
    {
        TextureDescriptor descriptor{};
        descriptor.samplerType = SamplerType::Sampler2D;
        descriptor.usage = TextureUsage::DefaultStorage | TextureUsage::Aliased;
        descriptor.format = TextureFormat::RGB9E5;
        descriptor.layers = 1;
        descriptor.levels = 8;
        descriptor.resolution = { initialWidth / 2u, initialHeight / 2u, 1u };
        descriptor.sampler.filterMin = FilterMode::Trilinear;
        descriptor.sampler.filterMag = FilterMode::Trilinear;

        m_bloomTexture = Texture::Create(descriptor, "Bloom.Texture");
        m_computeBloom = assetDatabase->Find<Shader>("CS_Bloom");
        m_passDownsample0 = m_computeBloom->GetVariantIndex(StringHashID::StringToID("PASS_DOWNSAMPLE0"));
        m_passDownsample = m_computeBloom->GetVariantIndex(StringHashID::StringToID("PASS_DOWNSAMPLE1"));
        m_passUpsample = m_computeBloom->GetVariantIndex(StringHashID::StringToID("PASS_UPSAMPLE"));
        m_passSeparableBlur = m_computeBloom->GetVariantIndex(StringHashID::StringToID("PASS_BLUR"));
    }

    void PassBloom::Render(Objects::CommandBuffer* cmd, RenderTexture* source)
    {
        cmd->BeginDebugScope("Bloom", PK_COLOR_MAGENTA);

        auto color = source->GetColor(0);
        auto bloom = m_bloomTexture.get();

        auto res = source->GetResolution();
        res.x >>= 1u;
        res.y >>= 1u;

        bloom->Validate(res);

        auto hash = HashCache::Get();

        GraphicsAPI::SetTexture(hash->pk_Texture, color, 0, 0);
        GraphicsAPI::SetImage(hash->pk_Image, bloom, 0, 0);
        GraphicsAPI::SetTexture(hash->pk_Bloom_Texture, bloom);
        
        cmd->Dispatch(m_computeBloom, m_passDownsample0, { res.x, res.y, 1u });

        for (auto i = 1u; i < 8u; ++i)
        {
            GraphicsAPI::SetTexture(hash->pk_Texture, bloom, i - 1u, 0);
            GraphicsAPI::SetImage(hash->pk_Image, bloom, i, 0);
            cmd->Dispatch(m_computeBloom, m_passDownsample, { res.x >> i, res.y >> i, 1u });
        }

        for (auto i = 6; i >= 0; --i)
        {
            GraphicsAPI::SetTexture(hash->pk_Texture, bloom, i + 1u, 0u);
            GraphicsAPI::SetImage(hash->pk_Image, bloom, i, 0u);
            cmd->Dispatch(m_computeBloom, m_passUpsample, { res.x >> i, res.y >> i, 1u });
        }

        cmd->EndDebugScope();
    }
}