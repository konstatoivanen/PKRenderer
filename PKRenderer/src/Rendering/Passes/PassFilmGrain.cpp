#include "PrecompiledHeader.h"
#include "PassFilmGrain.h"
#include "Rendering/HashCache.h"

namespace PK::Rendering::Passes
{
    using namespace Core;
    using namespace Core::Services;
    using namespace Math;
    using namespace Utilities;
    using namespace Structs;
    using namespace Objects;

    PassFilmGrain::PassFilmGrain(AssetDatabase* assetDatabase)
    {
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
        m_filmGrainTexture = Texture::Create(descriptor, "FilmGrain.Texture");
    }
    
    void PassFilmGrain::Compute(Objects::CommandBuffer* cmd)
    {
        auto hash = HashCache::Get();
        cmd->BeginDebugScope("Noise Compute", PK_COLOR32_BLUE);
        cmd->SetImage(hash->_MainTex, m_filmGrainTexture.get(), 0, 0);
        cmd->Dispatch(m_computeFilmGrain, { 16, 64, 1 });
        cmd->SetTexture(hash->pk_FilmGrainTex, m_filmGrainTexture.get());
        cmd->EndDebugScope();
    }
}