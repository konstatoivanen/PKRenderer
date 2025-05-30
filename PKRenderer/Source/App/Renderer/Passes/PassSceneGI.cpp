#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderPipelineBase.h"
#include "PassSceneGI.h"

namespace PK::App
{
    uint3 GetCheckerboardResolution(const uint3& resolution, bool halfRes)
    {
        return { resolution.x / (halfRes ? 2 : 1), resolution.y, resolution.z };
    }

    PassSceneGI::PassSceneGI(AssetDatabase* assetDatabase) 
    {
        PK_LOG_VERBOSE_FUNC("");

        m_computeClear = assetDatabase->Find<ShaderAsset>("CS_GI_Clear");
        m_computeMipmap = assetDatabase->Find<ShaderAsset>("CS_GI_VolumeMipmap");
        m_computeAccumulate = assetDatabase->Find<ShaderAsset>("CS_GI_Accumulate");
        m_computeShadeHits = assetDatabase->Find<ShaderAsset>("CS_GI_ShadeHits");
        m_computeReproject = assetDatabase->Find<ShaderAsset>("CS_GI_Reproject");
        m_computeGradients = assetDatabase->Find<ShaderAsset>("CS_GI_GradientEstimation");
        m_computePostFilter = assetDatabase->Find<ShaderAsset>("CS_GI_PostFilter");
        m_rayTraceGatherGI = assetDatabase->Find<ShaderAsset>("RS_GI_Raytrace");
        m_rayTraceValidate = assetDatabase->Find<ShaderAsset>("RS_GI_ValidateReservoirs");

        TextureDescriptor descr{};
        descr.type = TextureType::Texture3D;
        descr.format = TextureFormat::RGBA16F;
        descr.sampler.filterMin = FilterMode::Trilinear;
        descr.sampler.filterMag = FilterMode::Trilinear;
        descr.sampler.wrap[0] = WrapMode::Border;
        descr.sampler.wrap[1] = WrapMode::Border;
        descr.sampler.wrap[2] = WrapMode::Border;
        descr.sampler.borderColor = BorderColor::FloatClear;
        descr.resolution = { 256u, 128u, 256u };
        descr.levels = 7u;
        descr.usage = TextureUsage::Sample | TextureUsage::Storage;
        m_voxels = RHI::CreateTexture(descr, "GI.VoxelVolume");

        descr.format = TextureFormat::R8UI;
        descr.sampler.borderColor = BorderColor::IntClear;
        descr.levels = 1u;
        m_voxelMask = RHI::CreateTexture(descr, "GI.VoxelVolumeMask");

        m_voxelizeAttribs.depthStencil.depthCompareOp = Comparison::Off;
        m_voxelizeAttribs.depthStencil.depthWriteEnable = false;
        m_voxelizeAttribs.rasterization.cullMode = CullMode::Off;
        //m_voxelizeAttribs.rasterization.rasterMode = RasterMode::OverEstimate;

        auto hash = HashCache::Get();
        RHI::SetImage(hash->pk_GI_VolumeMaskWrite, m_voxelMask.get());
        RHI::SetImage(hash->pk_GI_VolumeWrite, m_voxels.get());
        RHI::SetTexture(hash->pk_GI_VolumeRead, m_voxels.get());
    }

    void PassSceneGI::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto resources = view->GetResources<ViewResources>();
        auto resolution = view->GetResolution();

        uint4 swizzles[3] =
        {
             { 0u, 2u, 1u, 0u },
             { 0u, 1u, 2u, 0u },
             { 1u, 2u, 0u, 0u },
        };

        const auto voxelSize = 0.6f;
        const auto angle = PK_FLOAT_PI / 3.0f;
        const auto levelscale = 2.0f * tan(angle / 2.0f) / voxelSize;
        const auto correctionAngle = tan(angle / 8.0f);
        const auto stepSize = (1.0f + correctionAngle) / (1.0f - correctionAngle) * voxelSize / 2.0f;

        view->constants->Set<float4>(hash->pk_GI_VolumeST, float4(-76.8f, -6.0f, -76.8f, 1.0f / voxelSize));
        view->constants->Set<float>(hash->pk_GI_VoxelSize, voxelSize);
        view->constants->Set<float>(hash->pk_GI_VoxelStepSize, stepSize);
        view->constants->Set<float>(hash->pk_GI_VoxelLevelScale, levelscale);

        auto frameIndexSinceResize = view->timeRender.frameIndex - view->timeResize.frameIndex;
        m_rasterAxis = frameIndexSinceResize % 3;
        view->constants->Set<uint4>(hash->pk_GI_VolumeSwizzle, swizzles[m_rasterAxis]);
        view->constants->Set<uint2>(hash->pk_GI_RayDither, Math::MurmurHash21(frameIndexSinceResize / 64u));

        RHI::SetKeyword("PK_GI_CHECKERBOARD_TRACE", m_settings.checkerboardTrace);
        RHI::SetKeyword("PK_GI_SPEC_VIRT_REPROJECT", m_settings.specularVirtualReproject);
        RHI::SetKeyword("PK_GI_SSRT_PRETRACE", m_settings.screenSpacePretrace);
        RHI::SetKeyword("PK_GI_RESTIR", m_settings.ReSTIR);

        auto* cmd = RHI::GetCommandBuffer(QueueType::Transfer);
        m_sbtRaytrace.Validate(cmd, m_rayTraceGatherGI);
        m_sbtValidate.Validate(cmd, m_rayTraceValidate);

        resources->hasResisedTargets = false;
        {
            TextureDescriptor descr{};
            descr.format = TextureFormat::RGBA32UI;
            descr.usage = TextureUsage::Sample | TextureUsage::Storage;
            descr.type = TextureType::Texture2DArray;
            descr.resolution = resolution;
            descr.layers = 2;
            descr.sampler.filterMin = FilterMode::Point;
            descr.sampler.filterMag = FilterMode::Point;
            descr.sampler.wrap[0] = WrapMode::Clamp;
            descr.sampler.wrap[1] = WrapMode::Clamp;
            descr.sampler.wrap[2] = WrapMode::Clamp;
            resources->hasResisedTargets |= RHI::ValidateTexture(resources->packedGIDiff, descr, "GI.PackedGI.Diff");

            descr.format = TextureFormat::RG32UI;
            resources->hasResisedTargets |= RHI::ValidateTexture(resources->packedGISpec, descr, "GI.PackedGI.Spec");

            descr.type = TextureType::Texture2D;
            descr.layers = 1u;
            descr.levels = 1u;
            descr.usage = TextureUsage::Storage;
            descr.format = TextureFormat::RG32UI;
            descr.resolution = GetCheckerboardResolution(descr.resolution, m_settings.checkerboardTrace);
            resources->hasResisedTargets |= RHI::ValidateTexture(resources->rayhits, descr, "GI.RayHits");

            descr.type = TextureType::Texture2DArray;
            descr.layers = 2;
            descr.format = TextureFormat::RGBA32UI;
            resources->hasResisedTargets |= RHI::ValidateTexture(resources->reservoirs0, descr, "GI.Reservoirs0");
            descr.format = TextureFormat::RG32UI;
            resources->hasResisedTargets |= RHI::ValidateTexture(resources->reservoirs1, descr, "GI.Reservoirs1");

            descr.type = TextureType::Texture2D;
            descr.layers = 1;
            descr.format = TextureFormat::RGBA32UI;
            descr.usage = TextureUsage::Storage | TextureUsage::Sample;
            descr.resolution = resolution;
            resources->hasResisedTargets |= RHI::ValidateTexture(resources->resolvedGI, descr, "GI.Resolved.DiffSpec");
        }

        RHI::SetImage(hash->pk_GI_RayHits, resources->rayhits.get());
        RHI::SetImage(hash->pk_Reservoirs0, resources->reservoirs0.get());
        RHI::SetImage(hash->pk_Reservoirs1, resources->reservoirs1.get());
        RHI::SetImage(hash->pk_GI_PackedDiff, resources->packedGIDiff.get());
        RHI::SetImage(hash->pk_GI_PackedSpec, resources->packedGISpec.get());
        RHI::SetImage(hash->pk_GI_ResolvedWrite, resources->resolvedGI.get());
        RHI::SetTexture(hash->pk_GI_ResolvedRead, resources->resolvedGI.get());
    }

    void PassSceneGI::PruneVoxels(CommandBufferExt cmd)
    {
        // Clear transparencies at the end of every axis cycle
        if (m_rasterAxis == 2)
        {
            cmd->BeginDebugScope("SceneGI.PruneVoxels", PK_COLOR_GREEN);
            RHI::SetImage(HashCache::Get()->pk_Image, m_voxels.get(), 0, 0);
            cmd.Dispatch(m_computeClear, m_voxels->GetResolution());
            cmd->EndDebugScope();
        }
    }

    void PassSceneGI::DispatchRays(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto view = context->views[0];
        auto resources = view->GetResources<ViewResources>();
        cmd->BeginDebugScope("SceneGI.DispatchRays", PK_COLOR_GREEN);
        cmd.SetShaderBindingTable(&m_sbtRaytrace);
        cmd.DispatchRays(m_rayTraceGatherGI, resources->rayhits->GetResolution());
        cmd->EndDebugScope();
    }

    void PassSceneGI::ReprojectGI(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        cmd->BeginDebugScope("SceneGI.Reproject", PK_COLOR_GREEN);
        auto view = context->views[0];
        auto resources = view->GetResources<ViewResources>();
        auto resolution = resources->packedGIDiff->GetResolution();
        cmd.Dispatch(m_computeReproject, { resolution.x, resolution.y, 1u });
        cmd->EndDebugScope();
    }

    void PassSceneGI::Voxelize(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto batcher = context->batcher;
        auto view = context->views[0];
        auto batchGroup = view->primaryPassGroup;
        auto resources = view->GetResources<ViewResources>();

        // Targets contain garbage data. skip this frame.
        if (!resources->hasResisedTargets)
        {
            cmd->BeginDebugScope("SceneGI.Voxelize", PK_COLOR_GREEN);

            auto hash = HashCache::Get();
            auto volumesize = m_voxels->GetResolution();

            uint4 viewports[3] =
            {
                {0u, 0u, volumesize.x, volumesize.z },
                {0u, 0u, volumesize.x, volumesize.y },
                {0u, 0u, volumesize.y, volumesize.z },
            };

            // Voxelize raster
            cmd.SetRenderTarget({ viewports[m_rasterAxis].z, viewports[m_rasterAxis].w }, 1 );
            cmd.SetViewPort(viewports[m_rasterAxis]);
            cmd.SetScissor(viewports[m_rasterAxis]);
            batcher->RenderGroup(cmd, batchGroup, &m_voxelizeAttribs, hash->PK_META_PASS_GIVOXELIZE);

            cmd->EndDebugScope();
        }
    }

    void PassSceneGI::RenderGI(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto view = context->views[0];
        auto resources = view->GetResources<ViewResources>();
        auto resolution = resources->packedGIDiff->GetResolution();
        uint3 dimension = { resolution.x, resolution.y, 1u };
        uint3 chbdimension = GetCheckerboardResolution(dimension, m_settings.checkerboardTrace);
        cmd->BeginDebugScope("SceneGI.Filter", PK_COLOR_GREEN);
        cmd.Dispatch(m_computeShadeHits, chbdimension);
        cmd.Dispatch(m_computeAccumulate, chbdimension);
        cmd.Dispatch(m_computePostFilter, m_settings.checkerboardTrace ? 1 : 0, chbdimension);
        cmd->EndDebugScope();
    }

    void PassSceneGI::VoxelMips(CommandBufferExt cmd)
    {
        // Voxel mips
        auto hash = HashCache::Get();
        auto volumesize = m_voxels->GetResolution();
        RHI::SetTexture(hash->pk_Texture, m_voxels.get());
        RHI::SetImage(hash->pk_Image, m_voxels.get(), 1, 0);
        RHI::SetImage(hash->pk_Image1, m_voxels.get(), 2, 0);
        RHI::SetImage(hash->pk_Image2, m_voxels.get(), 3, 0);
        cmd.Dispatch(m_computeMipmap, 0, volumesize >> 1u);
        RHI::SetImage(hash->pk_Image, m_voxels.get(), 4, 0);
        RHI::SetImage(hash->pk_Image1, m_voxels.get(), 5, 0);
        RHI::SetImage(hash->pk_Image2, m_voxels.get(), 6, 0);
        cmd.Dispatch(m_computeMipmap, 0, volumesize >> 4u);
    }

    void PassSceneGI::ValidateReservoirs(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        if (m_settings.ReSTIR)
        {
            auto view = context->views[0];
            auto resources = view->GetResources<ViewResources>();
            cmd->BeginDebugScope("SceneGI.ValidateReservoirs", PK_COLOR_GREEN);
            cmd.SetShaderBindingTable(&m_sbtValidate);
            cmd.DispatchRays(m_rayTraceValidate, resources->reservoirs0->GetResolution());
            cmd->EndDebugScope();
        }
    }
}