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
        descriptor.usage = TextureUsage::DefaultStorage;
        descriptor.format = TextureFormat::RGBA16F;
        descriptor.layers = 2;
        descriptor.levels = 6;
        descriptor.resolution.x = initialWidth / 2;
        descriptor.resolution.y = initialWidth / 2;
        descriptor.sampler.filter = FilterMode::Trilinear;

        m_bloomTexture = Texture::Create(descriptor, "Bloom Texture");
        m_computeBloom = assetDatabase->Find<Shader>("CS_Bloom");
        m_passPrefilter = m_computeBloom->GetVariantIndex(StringHashID::StringToID("PASS_DOWNSAMPLE"));
        m_passDiskblur = m_computeBloom->GetVariantIndex(StringHashID::StringToID("PASS_BLUR"));
    }
    
    void PassBloom::Execute(RenderTexture* source, MemoryAccessFlags lastAccess)
    {
        auto cmd = GraphicsAPI::GetCommandBuffer();

        auto color = source->GetColor(0);
        auto bloom = m_bloomTexture.get();

        cmd->Barrier(color, lastAccess, MemoryAccessFlags::ComputeReadWrite);

        auto res = source->GetResolution();
        res.x /= 2;
        res.y /= 2;

        uint3 groups[6];

        for (auto i = 0; i < 6; ++i)
        {
            groups[i] = { (uint)glm::ceil((res.x >> i) / 16.0f), (uint)glm::ceil((res.y >> i) / 4.0f), 1 };
        }

        bloom->Validate(res);

        auto hash = HashCache::Get();
        auto ls = 0u;
        auto ld = 1u;

        for (auto i = 0u; i < 6u; ++i)
        {
            cmd->SetTexture(hash->_SourceTex, i == 0 ? color : bloom, i == 0 ? 0 : i - 1u, ls);
            cmd->SetImage(hash->_DestinationTex, bloom, i, ld);
            cmd->Dispatch(m_computeBloom, m_passPrefilter, groups[i]);
            cmd->Barrier(bloom, i, ld, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::ComputeRead);
            ls ^= 1u;
            ld ^= 1u;

            cmd->SetTexture(hash->_SourceTex, bloom, i, ls);
            cmd->SetImage(hash->_DestinationTex, bloom, i, ld);
            cmd->SetConstant<float2>(hash->_BlurOffset, { 1.0f, 0.0f });
            cmd->Dispatch(m_computeBloom, m_passDiskblur, groups[i]);
            cmd->Barrier(bloom, i, ld, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::ComputeRead);
            ls ^= 1u;
            ld ^= 1u;

            cmd->SetTexture(hash->_SourceTex, bloom, i, ls);
            cmd->SetImage(hash->_DestinationTex, bloom, i, ld);
            cmd->SetConstant<float2>(hash->_BlurOffset, { 0.0f, 1.0f });
            cmd->Dispatch(m_computeBloom, m_passDiskblur, groups[i]);
            cmd->Barrier(bloom, i, ld, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::ComputeRead);
            ls ^= 1u;
            ld ^= 1u;
        }

        cmd->SetTexture(hash->pk_BloomTexture, bloom, { 0, 1, 6, 1 });
        cmd->SetTexture(hash->pk_BloomTexture1, bloom, { 0, 0, 6, 1 });
    }
}