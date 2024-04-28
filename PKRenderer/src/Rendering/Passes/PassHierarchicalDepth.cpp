#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/RHI/Objects/Shader.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "Rendering/HashCache.h"
#include "PassHierarchicalDepth.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::Services;
    using namespace PK::Core::Assets;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    PassHierarchicalDepth::PassHierarchicalDepth(AssetDatabase* assetDatabase, const ApplicationConfig* config)
    {
        PK_LOG_VERBOSE("PassHierarchicalDepth.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_computeHierachicalDepth = assetDatabase->Find<Shader>("CS_HierachicalDepth");

        TextureDescriptor hizDesc{};
        hizDesc.samplerType = SamplerType::Sampler2DArray;
        hizDesc.format = TextureFormat::R16F;
        hizDesc.sampler.filterMin = FilterMode::Bilinear;
        hizDesc.sampler.filterMag = FilterMode::Bilinear;
        hizDesc.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        hizDesc.levels = 9u;
        hizDesc.layers = 4u;//2
        hizDesc.usage = TextureUsage::Sample | TextureUsage::Storage;
        m_hierarchicalDepth = Texture::Create(hizDesc, "Scene.HierarchicalDepth");
    }
    
    void PassHierarchicalDepth::Compute(RHI::Objects::CommandBuffer* cmd, uint3 resolution)
    {
        auto hash = HashCache::Get();

        m_hierarchicalDepth->Validate({ resolution.x, resolution.y, 1u });
        GraphicsAPI::SetTexture(hash->pk_GB_Current_DepthMips, m_hierarchicalDepth.get());

        resolution.x >>= 1u;
        resolution.y >>= 1u;
        GraphicsAPI::SetImage(hash->pk_Image, m_hierarchicalDepth.get(), { 0, 0, 1, 2 });
        GraphicsAPI::SetImage(hash->pk_Image1, m_hierarchicalDepth.get(), { 1, 0, 1, 2 });
        GraphicsAPI::SetImage(hash->pk_Image2, m_hierarchicalDepth.get(), { 2, 0, 1, 2 });
        GraphicsAPI::SetImage(hash->pk_Image3, m_hierarchicalDepth.get(), { 3, 0, 1, 2 });
        GraphicsAPI::SetImage(hash->pk_Image4, m_hierarchicalDepth.get(), { 4, 0, 1, 2 });
        cmd->Dispatch(m_computeHierachicalDepth, 0u, resolution);

        resolution.x >>= 4u;
        resolution.y >>= 4u;
        GraphicsAPI::SetImage(hash->pk_Image1, m_hierarchicalDepth.get(), { 5, 0, 1, 2 });
        GraphicsAPI::SetImage(hash->pk_Image2, m_hierarchicalDepth.get(), { 6, 0, 1, 2 });
        GraphicsAPI::SetImage(hash->pk_Image3, m_hierarchicalDepth.get(), { 7, 0, 1, 2 });
        GraphicsAPI::SetImage(hash->pk_Image4, m_hierarchicalDepth.get(), { 8, 0, 1, 2 });
        cmd->Dispatch(m_computeHierachicalDepth, 1u, resolution);
    }
}