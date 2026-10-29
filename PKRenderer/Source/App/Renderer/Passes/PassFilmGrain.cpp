#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/RenderGraphExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "PassFilmGrain.h"

namespace PK::App
{
    PassFilmGrain::PassFilmGrain(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE_FUNC();

        m_computeFilmGrain = assetDatabase->Find<ShaderAsset>("CS_FilmGrain").get();

        TextureDescriptor descriptor{};
        descriptor.format = TextureFormat::RGBA8_Unorm;
        descriptor.resolution.x = 256;
        descriptor.resolution.y = 256;
        descriptor.usage = TextureUsage::DefaultStorage;
        m_filmGrainTexture = RHI::CreateTexture(descriptor, "FilmGrain.Texture");
        RHI::SetTexture(HashCache::Get()->pk_FilmGrain_Texture, m_filmGrainTexture.get());
    }

    void PassFilmGrain::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.FilmGrainSettings;
        view->constants.Set<float>(hash->FilmGrain_Luminance, settings.Luminance);
        view->constants.Set<float>(hash->FilmGrain_Intensity, settings.Intensity);
        view->constants.Set<float>(hash->FilmGrain_ExposureSensitivity, settings.ExposureSensitivity);
    }

    void PassFilmGrain::Compute(RenderGraphExt graph, QueueType queue)
    {
        auto hash = HashCache::Get();
        graph->BeginDebugScope("Noise Compute", float4(PK_COLOR32_BLUE));
        RHI::SetImage(hash->pk_Image, m_filmGrainTexture.get(), 0, 0);
        graph.Dispatch(queue, m_computeFilmGrain, { 256, 256, 1 });
        graph->EndDebugScope();
    }
}
