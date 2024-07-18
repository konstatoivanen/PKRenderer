#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "App/Renderer/HashCache.h"
#include "PassHierarchicalDepth.h"

namespace PK::App
{
    PassHierarchicalDepth::PassHierarchicalDepth(AssetDatabase* assetDatabase, const uint2& initialResolution)
    {
        PK_LOG_VERBOSE("PassHierarchicalDepth.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_computeHierachicalDepth = assetDatabase->Find<ShaderAsset>("CS_HierachicalDepth");

        TextureDescriptor hizDesc{};
        hizDesc.type = TextureType::Texture2DArray;
        hizDesc.format = TextureFormat::R16F;
        hizDesc.sampler.filterMin = FilterMode::Bilinear;
        hizDesc.sampler.filterMag = FilterMode::Bilinear;
        hizDesc.resolution = { initialResolution, 1u };
        hizDesc.levels = 9u;
        hizDesc.layers = 4u;//2
        hizDesc.usage = TextureUsage::Sample | TextureUsage::Storage;
        m_hierarchicalDepth = RHI::CreateTexture(hizDesc, "Scene.HierarchicalDepth");
    }

    void PassHierarchicalDepth::Compute(CommandBufferExt cmd, uint3 resolution)
    {
        auto hash = HashCache::Get();

        RHI::ValidateTexture(m_hierarchicalDepth, { resolution.x, resolution.y, 1u });
        RHI::SetTexture(hash->pk_GB_Current_DepthMips, m_hierarchicalDepth.get());

        resolution.x >>= 1u;
        resolution.y >>= 1u;
        RHI::SetImage(hash->pk_Image, m_hierarchicalDepth.get(), { 0, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image1, m_hierarchicalDepth.get(), { 1, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image2, m_hierarchicalDepth.get(), { 2, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image3, m_hierarchicalDepth.get(), { 3, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image4, m_hierarchicalDepth.get(), { 4, 0, 1, 2 });
        cmd.Dispatch(m_computeHierachicalDepth, 0u, resolution);

        resolution.x >>= 4u;
        resolution.y >>= 4u;
        RHI::SetImage(hash->pk_Image1, m_hierarchicalDepth.get(), { 5, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image2, m_hierarchicalDepth.get(), { 6, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image3, m_hierarchicalDepth.get(), { 7, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image4, m_hierarchicalDepth.get(), { 8, 0, 1, 2 });
        cmd.Dispatch(m_computeHierachicalDepth, 1u, resolution);
    }
}