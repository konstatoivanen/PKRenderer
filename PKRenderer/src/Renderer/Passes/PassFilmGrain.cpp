#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Graphics/RHI/RHITexture.h"
#include "Graphics/RHI/RHICommandBuffer.h"
#include "Graphics/CommandBufferExt.h"
#include "Graphics/Shader.h"
#include "Renderer/HashCache.h"
#include "PassFilmGrain.h"

namespace PK::Renderer::Passes
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::Graphics;
    using namespace PK::Graphics::RHI;

    PassFilmGrain::PassFilmGrain(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE("PassFilmGrain.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_computeFilmGrain = assetDatabase->Find<Shader>("CS_FilmGrain");

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
        m_filmGrainTexture = RHICreateTexture(descriptor, "FilmGrain.Texture");
        RHISetTexture(HashCache::Get()->pk_FilmGrain_Texture, m_filmGrainTexture.get());
    }

    void PassFilmGrain::Compute(CommandBufferExt cmd)
    {
        auto hash = HashCache::Get();
        cmd->BeginDebugScope("Noise Compute", PK_COLOR32_BLUE);
        RHISetImage(hash->pk_Image, m_filmGrainTexture.get(), 0, 0);
        cmd.Dispatch(m_computeFilmGrain, { 256, 256, 1 });
        cmd->EndDebugScope();
    }
}