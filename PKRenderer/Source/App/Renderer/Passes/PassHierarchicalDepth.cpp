#include "PrecompiledHeader.h"
#include "Core/Math/Extended.h"
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
        PK_LOG_VERBOSE_FUNC();
        m_computeHierachicalDepth = assetDatabase->Find<ShaderAsset>("CS_HierarchicalDepth").get();
        m_worgroupCounter = RHI::CreateBuffer(sizeof(uint32_t), BufferUsage::DefaultStorage, "HierarchicalDepth.AtomicCounter");
        RHI::SetTexture(HashCache::Get()->pk_GB_Current_DepthMips, RHI::GetBuiltInResources()->BlackTexture2DArray.get());
        RHI::SetBuffer(HashCache::Get()->pk_HZB_WorkgroupCounter, m_worgroupCounter.get());
    }

    void PassHierarchicalDepth::SetViewConstants([[maybe_unused]] RenderView* view)
    {
        RHI::GetQueues()->GetCommandBuffer(QueueType::Transfer)->Clear(m_worgroupCounter.get(), 0, sizeof(uint32_t), 0u);
    }

    void PassHierarchicalDepth::Compute(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto hash = HashCache::Get();
        auto view = context->views[0];
        auto resources = view->GetResources<ViewResources>();
        auto resolution = view->GetResolution();

        TextureDescriptor hzbDesc{};
        hzbDesc.type = TextureType::Texture2DArray;
        hzbDesc.format = TextureFormat::R16F;
        hzbDesc.sampler.filterMin = FilterMode::Bilinear;
        hzbDesc.sampler.filterMag = FilterMode::Bilinear;
        hzbDesc.resolution = resolution;
        hzbDesc.levels = (uint8_t)math::min(13u, math::levels(uint2(resolution.x, resolution.y)));
        hzbDesc.layers = 2u;
        hzbDesc.usage = TextureUsage::Sample | TextureUsage::Storage;
        RHI::ValidateTexture(resources->hierarchicalDepth, hzbDesc, "Scene.HierarchicalDepth");
        RHI::SetTexture(hash->pk_GB_Current_DepthMips, resources->hierarchicalDepth.get());

        const auto endIndexX = (resolution.x - 1) / 64;
        const auto endIndexY = (resolution.y - 1) / 64;
        const auto groupCountX = endIndexX + 1;
        const auto groupCountY = endIndexY + 1;
        const auto numWorkGroups = groupCountX * groupCountY;
        
        RHI::SetConstant<uint4>(hash->pk_HZB_Parameters, uint4(hzbDesc.levels, numWorkGroups, resolution.xy));

        // We have at least 8 mips based on min window scale. All targets need to be bound though, rebind last mip to fill the rest of the bindings.
        RHI::SetImage(hash->pk_Image, resources->hierarchicalDepth.get(), { 0, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image1, resources->hierarchicalDepth.get(), { 1, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image2, resources->hierarchicalDepth.get(), { 2, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image3, resources->hierarchicalDepth.get(), { 3, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image4, resources->hierarchicalDepth.get(), { 4, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image5, resources->hierarchicalDepth.get(), { 5, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image6, resources->hierarchicalDepth.get(), { 6, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image7, resources->hierarchicalDepth.get(), { 7, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image8, resources->hierarchicalDepth.get(), { 8, 0, 1, 2 });
        RHI::SetImage(hash->pk_Image9, resources->hierarchicalDepth.get(), { (uint16_t)math::min(9u, hzbDesc.levels - 1u), 0, 1, 2 });
        RHI::SetImage(hash->pk_Image10, resources->hierarchicalDepth.get(), { (uint16_t)math::min(10u, hzbDesc.levels - 1u), 0, 1, 2 });
        RHI::SetImage(hash->pk_Image11, resources->hierarchicalDepth.get(), { (uint16_t)math::min(11u, hzbDesc.levels - 1u), 0, 1, 2 });
        RHI::SetImage(hash->pk_Image12, resources->hierarchicalDepth.get(), { (uint16_t)math::min(12u, hzbDesc.levels - 1u), 0, 1, 2 });
        cmd.Dispatch(m_computeHierachicalDepth, 0u, uint3(256u * groupCountX, groupCountY, 1));
    }
}
