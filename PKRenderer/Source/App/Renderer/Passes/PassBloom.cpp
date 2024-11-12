#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/TextureAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderViewSettings.h"
#include "PassBloom.h"

namespace PK::App
{
    PassBloom::PassBloom(AssetDatabase* assetDatabase, const uint2& initialResolution)
    {
        PK_LOG_VERBOSE("PassBloom.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        TextureDescriptor descriptor{};
        descriptor.type = TextureType::Texture2D;
        descriptor.usage = TextureUsage::DefaultStorage;
        descriptor.format = TextureFormat::RGB9E5;
        descriptor.formatAlias = TextureFormat::R32UI;
        descriptor.layers = 1;
        descriptor.levels = Math::GetMaxMipLevel(initialResolution / 2u);
        descriptor.resolution = { initialResolution / 2u, 1u };
        descriptor.sampler.filterMin = FilterMode::Trilinear;
        descriptor.sampler.filterMag = FilterMode::Trilinear;
        descriptor.sampler.wrap[0] = WrapMode::Border;
        descriptor.sampler.wrap[1] = WrapMode::Border;
        descriptor.sampler.wrap[2] = WrapMode::Border;
        descriptor.sampler.borderColor = BorderColor::FloatBlack;

        m_bloomTexture = RHI::CreateTexture(descriptor, "Bloom.Texture");
        m_computeBloom = assetDatabase->Find<ShaderAsset>("CS_Bloom");
        m_passDownsample0 = m_computeBloom->GetRHIIndex("PASS_DOWNSAMPLE0");
        m_passDownsample = m_computeBloom->GetRHIIndex("PASS_DOWNSAMPLE1");
        m_passUpsample = m_computeBloom->GetRHIIndex("PASS_UPSAMPLE");
    }

    void PassBloom::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.BloomSettings;
        m_bloomLensDirtTexture = settings.LensDirtTextureAsset ? settings.LensDirtTextureAsset->GetRHI() : RHI::GetBuiltInResources()->WhiteTexture2D.get();
        view->constants->Set<float>(hash->pk_Bloom_Diffusion, settings.Diffusion);
        view->constants->Set<float>(hash->pk_Bloom_Intensity, glm::clamp(glm::exp(settings.Intensity) - 1.0f, 0.0f, 1.0f));
        view->constants->Set<float>(hash->pk_Bloom_DirtIntensity, glm::clamp(glm::exp(settings.LensDirtIntensity) - 1.0f, 0.0f, 1.0f));
        RHI::SetTexture(HashCache::Get()->pk_Bloom_LensDirtTex, m_bloomLensDirtTexture);

        auto sampler = m_bloomTexture->GetSamplerDescriptor();
        auto wrapMode = settings.BorderClip ? WrapMode::Border : WrapMode::Clamp;

        if (sampler.wrap[0] != wrapMode) 
        {
            sampler.wrap[0] = wrapMode;
            sampler.wrap[1] = wrapMode;
            sampler.wrap[2] = wrapMode;
            m_bloomTexture->SetSampler(sampler);
        }
    }

    void PassBloom::Render(CommandBufferExt cmd, RHITexture* source)
    {
        cmd->BeginDebugScope("Bloom", PK_COLOR_MAGENTA);

        auto res = source->GetResolution();
        res.x >>= 1u;
        res.y >>= 1u;

        auto leveCount = Math::GetMaxMipLevel(uint2(res.x, res.y));

        RHI::ValidateTexture(m_bloomTexture, res, leveCount);
        
        auto bloom = m_bloomTexture.get();
        auto hash = HashCache::Get();

        RHI::SetTexture(hash->pk_Texture, source, 0, 0);
        RHI::SetImage(hash->pk_Image, bloom, 0, 0);
        RHI::SetTexture(hash->pk_Bloom_Texture, bloom);

        cmd.Dispatch(m_computeBloom, m_passDownsample0, { res.x, res.y, 1u });

        for (auto i = 1u; i < leveCount; ++i)
        {
            RHI::SetTexture(hash->pk_Texture, bloom, i - 1u, 0);
            RHI::SetImage(hash->pk_Image, bloom, i, 0);
            cmd.Dispatch(m_computeBloom, m_passDownsample, { res.x >> i, res.y >> i, 1u });
        }

        for (auto i = int(leveCount) - 2; i >= 0; --i)
        {
            RHI::SetConstant(hash->pk_Bloom_UpsampleLayerCount, float(leveCount) - (i + 1.0f));
            RHI::SetTexture(hash->pk_Texture, bloom, i + 1u, 0u);
            RHI::SetImage(hash->pk_Image, bloom, i, 0u);
            cmd.Dispatch(m_computeBloom, m_passUpsample, { res.x >> i, res.y >> i, 1u });
        }

        cmd->EndDebugScope();
    }
}