#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderPipelineBase.h"
#include "PassHierarchicalDepth.h"

namespace PK::App
{
    PassHierarchicalDepth::PassHierarchicalDepth(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE("PassHierarchicalDepth.Ctor");
        PK_LOG_SCOPE_INDENT(local);
        m_computeHierachicalDepth = assetDatabase->Find<ShaderAsset>("CS_HierachicalDepth");
        RHI::SetTexture(HashCache::Get()->pk_GB_Current_DepthMips, RHI::GetBuiltInResources()->BlackTexture2DArray.get());
    }

    void PassHierarchicalDepth::Compute(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto hash = HashCache::Get();
        auto view = context->views[0];
        auto resources = view->GetResources<ViewResources>();
        auto resolution = view->GetResolution();

        TextureDescriptor hizDesc{};
        hizDesc.type = TextureType::Texture2DArray;
        hizDesc.format = TextureFormat::R16F;
        hizDesc.sampler.filterMin = FilterMode::Bilinear;
        hizDesc.sampler.filterMag = FilterMode::Bilinear;
        hizDesc.resolution = resolution;
        hizDesc.levels = 9u;
        hizDesc.layers = 4u;
        hizDesc.usage = TextureUsage::Sample | TextureUsage::Storage;
        RHI::ValidateTexture(resources->hierarchicalDepth, hizDesc, "Scene.HierarchicalDepth");
        RHI::SetTexture(hash->pk_GB_Current_DepthMips, resources->hierarchicalDepth.get());

        resolution.x >>= 1u;
        resolution.y >>= 1u;
        RHI::SetImage(hash->pk_Image, resources->hierarchicalDepth.get(), { 0, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image1, resources->hierarchicalDepth.get(), { 1, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image2, resources->hierarchicalDepth.get(), { 2, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image3, resources->hierarchicalDepth.get(), { 3, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image4, resources->hierarchicalDepth.get(), { 4, 0, 1, 2 });
        cmd.Dispatch(m_computeHierachicalDepth, 0u, resolution);

        resolution.x >>= 4u;
        resolution.y >>= 4u;
        RHI::SetImage(hash->pk_Image1, resources->hierarchicalDepth.get(), { 5, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image2, resources->hierarchicalDepth.get(), { 6, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image3, resources->hierarchicalDepth.get(), { 7, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image4, resources->hierarchicalDepth.get(), { 8, 0, 1, 2 });
        cmd.Dispatch(m_computeHierachicalDepth, 1u, resolution);
    }
}