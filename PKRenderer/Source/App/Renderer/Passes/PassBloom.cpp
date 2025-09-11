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
#include "App/Renderer/RenderPipelineBase.h"
#include "PassBloom.h"

namespace PK::App
{
    PassBloom::PassBloom(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE_FUNC("");
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
    }

    void PassBloom::Render(CommandBufferExt cmd, RenderPipelineContext* ctx)
    {
        cmd->BeginDebugScope("Bloom", PK_COLOR_MAGENTA);

        auto view = ctx->views[0];
        auto source = view->gbuffers.GetView().current.color;
        auto resources = view->GetResources<ViewResources>();
        auto resolution = source->GetResolution();
        resolution.x >>= 1u;
        resolution.y >>= 1u;

        auto leveCount = Math::GetMaxMipLevel(uint2(resolution.x, resolution.y));

        {
            TextureDescriptor descriptor{};
            descriptor.type = TextureType::Texture2D;
            descriptor.usage = TextureUsage::DefaultStorage;
            descriptor.format = TextureFormat::RGB9E5;
            descriptor.formatAlias = TextureFormat::R32UI;
            descriptor.layers = 1;
            descriptor.levels = leveCount;
            descriptor.resolution = resolution;
            descriptor.sampler.filterMin = FilterMode::Trilinear;
            descriptor.sampler.filterMag = FilterMode::Trilinear;
            descriptor.sampler.wrap[0] = view->settings.BloomSettings.BorderClip ? WrapMode::Border : WrapMode::Clamp;
            descriptor.sampler.wrap[1] = view->settings.BloomSettings.BorderClip ? WrapMode::Border : WrapMode::Clamp;
            descriptor.sampler.wrap[2] = view->settings.BloomSettings.BorderClip ? WrapMode::Border : WrapMode::Clamp;
            descriptor.sampler.borderColor = BorderColor::FloatBlack;
            RHI::ValidateTexture(resources->bloomTexture, descriptor, "Bloom.Texture");
        }
        
        auto bloom = resources->bloomTexture.get();
        auto hash = HashCache::Get();

        RHI::SetTexture(hash->pk_Texture, source, 0, 0);
        RHI::SetImage(hash->pk_Image, bloom, 0, 0);
        RHI::SetTexture(hash->pk_Bloom_Texture, bloom);

        cmd.Dispatch(m_computeBloom, m_passDownsample0, { resolution.x, resolution.y, 1u });

        for (auto i = 1u; i < leveCount; ++i)
        {
            RHI::SetTexture(hash->pk_Texture, bloom, i - 1u, 0);
            RHI::SetImage(hash->pk_Image, bloom, i, 0);
            cmd.Dispatch(m_computeBloom, m_passDownsample, { resolution.x >> i, resolution.y >> i, 1u });
        }

        for (auto i = int(leveCount) - 2; i >= 0; --i)
        {
            RHI::SetConstant(hash->pk_Bloom_UpsampleLayerCount, float(leveCount) - (i + 1.0f));
            RHI::SetTexture(hash->pk_Texture, bloom, i + 1u, 0u);
            RHI::SetImage(hash->pk_Image, bloom, i, 0u);
            cmd.Dispatch(m_computeBloom, m_passUpsample, { resolution.x >> i, resolution.y >> i, 1u });
        }

        cmd->EndDebugScope();
    }
}
