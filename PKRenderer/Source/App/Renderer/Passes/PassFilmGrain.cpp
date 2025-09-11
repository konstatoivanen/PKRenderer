#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderViewSettings.h"
#include "PassFilmGrain.h"

namespace PK::App
{
    PassFilmGrain::PassFilmGrain(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE_FUNC("");

        m_computeFilmGrain = assetDatabase->Find<ShaderAsset>("CS_FilmGrain");

        TextureDescriptor descriptor{};
        descriptor.format = TextureFormat::RGBA8;
        descriptor.resolution.x = 256;
        descriptor.resolution.y = 256;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;
        descriptor.sampler.wrap[0] = WrapMode::Repeat;
        descriptor.sampler.wrap[1] = WrapMode::Repeat;
        descriptor.sampler.wrap[2] = WrapMode::Repeat;
        descriptor.usage = TextureUsage::DefaultStorage | TextureUsage::Concurrent;
        m_filmGrainTexture = RHI::CreateTexture(descriptor, "FilmGrain.Texture");
        RHI::SetTexture(HashCache::Get()->pk_FilmGrain_Texture, m_filmGrainTexture.get());
    }

    void PassFilmGrain::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.FilmGrainSettings;
        view->constants->Set<float>(hash->pk_FilmGrain_Luminance, settings.Luminance);
        view->constants->Set<float>(hash->pk_FilmGrain_Intensity, settings.Intensity);
        view->constants->Set<float>(hash->pk_FilmGrain_ExposureSensitivity, settings.ExposureSensitivity);
    }

    void PassFilmGrain::Compute(CommandBufferExt cmd)
    {
        auto hash = HashCache::Get();
        cmd->BeginDebugScope("Noise Compute", PK_COLOR32_BLUE);
        RHI::SetImage(hash->pk_Image, m_filmGrainTexture.get(), 0, 0);
        cmd.Dispatch(m_computeFilmGrain, { 256, 256, 1 });
        cmd->EndDebugScope();
    }
}
