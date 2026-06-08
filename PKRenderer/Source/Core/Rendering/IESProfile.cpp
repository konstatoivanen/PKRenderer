#include "PrecompiledHeader.h"
#include <PKAssets/PKAssetLoader.h>
#include "Core/Math/Color.h"
#include "Core/ControlFlow/FenceRef.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Structs.h"
#include "Core/RHI/RHInterfaces.h"
#include "IESProfile.h"

namespace PK
{
    IESProfileAtlas::IESProfileAtlas(size_t initialCapacity) : m_residency(initialCapacity)
    {
        Reserve(initialCapacity);
    }

    void IESProfileAtlas::Reserve(size_t capacity)
    {
        if (m_texture == nullptr || m_texture->GetLayers() < capacity)
        {
            TextureDescriptor desc{};
            desc.format = PKAssets::PK_IES_PROFILE_FORMAT;
            desc.usage = TextureUsage::DefaultDisk;
            desc.type = TextureType::Texture2DArray;
            desc.resolution = { PKAssets::PK_IES_PROFILE_WIDTH, PKAssets::PK_IES_PROFILE_HEIGHT, 1u };
            desc.levels = 1u;
            desc.samples = 1u;
            desc.layers = capacity;
            desc.sampler.filterMin = FilterMode::Bilinear;
            desc.sampler.filterMag = FilterMode::Bilinear;
            auto newTexture = RHI::CreateTexture(desc, "IESProfile.Atlas");

            if (m_texture != nullptr)
            {
                auto cmd = RHI::GetCommandBuffer(QueueType::Transfer);
                cmd->Blit(m_texture.get(), newTexture.get(), {}, {}, FilterMode::Point);
            }

            m_texture = newTexture;
            m_residency.Reserve(capacity, true);
        }
    }

    void IESProfileAtlas::AssetConstruct(IESProfile* memory, const char* filepath)
    {
        PKAssets::PKAsset asset{};
        PK_FATAL_ASSERT(PKAssets::OpenAsset(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_FATAL_ASSERT(asset.header->type == PKAssets::PKAssetType::IESProfile, "Trying to read a IES profile from a non IES profile file!")

        auto index = m_residency.FindFirstZero();

        if (index == -1)
        {
            Reserve(m_residency.GetCapacity() + 1u);
            index = m_residency.FindFirstZero();
        }

        m_residency.FlipAt(index);

        auto profile = PKAssets::ReadAsIESProfile(&asset);

        TextureDataRegion region{};
        region.layer = static_cast<uint64_t>(index);
        region.layers = 1ull;
        region.offset = PK_UINT3_ZERO;
        region.extent = m_texture->GetResolution();

        auto cmd = RHI::GetCommandBuffer(QueueType::Transfer);
        cmd->CopyToTexture(m_texture.get(), profile->data.Get(asset.rawData), PKAssets::PK_IES_PROFILE_DATA_SIZE, &region, 1u);

        Memory::Construct(memory, this, static_cast<uint32_t>(index) + 1u, profile->lumens, profile->candelaMax, profile->candelaAverage);
        PKAssets::CloseAsset(&asset);
    }

    void IESProfileAtlas::AssetDestruct(uint32_t index)
    {
        if (index > 0u)
        {
            m_residency.FlipAt(index - 1u);
        }
    }

    RHITexture* IESProfileAtlas::GetRHI() { return m_texture.get(); }
    
    const RHITexture* IESProfileAtlas::GetRHI() const { return m_texture.get(); }
    
    IESProfileAtlas::operator RHITexture* () { return m_texture.get(); }

    IESProfileAtlas::operator const RHITexture* () const { return m_texture.get(); }
    
    color IESProfile::PreprocessColor(const color& input, bool applyProfileCandelas) const
    {
        float3 output = input.rgb;

        if (applyProfileCandelas)
        {
            output = math::normalizeColor(input).rgb;
            output *= m_candelaMax;
        }
        else
        {
            output *= m_candelaMax / m_candelaAverage;
        }

        return color(output, input.a);
    }
}
