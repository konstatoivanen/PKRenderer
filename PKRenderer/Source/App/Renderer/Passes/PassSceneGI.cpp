#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/RendererConfig.h"
#include "App/Renderer/HashCache.h"
#include "PassSceneGI.h"

namespace PK::App
{
    uint3 GetCheckerboardResolution(const uint3& resolution, bool halfRes)
    {
        return { resolution.x / (halfRes ? 2 : 1), resolution.y, resolution.z };
    }

    PassSceneGI::PassSceneGI(AssetDatabase* assetDatabase, const RendererConfig* config)
    {
        PK_LOG_VERBOSE("PassSceneGI.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_computeClear = assetDatabase->Find<ShaderAsset>("CS_GI_Clear");
        m_computeMipmap = assetDatabase->Find<ShaderAsset>("CS_GI_VolumeMipmap");
        m_computeAccumulate = assetDatabase->Find<ShaderAsset>("CS_GI_Accumulate");
        m_computeShadeHits = assetDatabase->Find<ShaderAsset>("CS_GI_ShadeHits");
        m_computeReproject = assetDatabase->Find<ShaderAsset>("CS_GI_Reproject");
        m_computeGradients = assetDatabase->Find<ShaderAsset>("CS_GI_GradientEstimation");
        m_computePostFilter = assetDatabase->Find<ShaderAsset>("CS_GI_PostFilter");
        m_rayTraceGatherGI = assetDatabase->Find<ShaderAsset>("RS_GI_Raytrace");
        m_rayTraceValidate = assetDatabase->Find<ShaderAsset>("RS_GI_ValidateReservoirs");
        OnUpdateParameters(config);

        TextureDescriptor descr{};
        descr.samplerType = SamplerType::Sampler3D;
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

        descr.samplerType = SamplerType::Sampler2DArray;
        descr.sampler.wrap[0] = WrapMode::Clamp;
        descr.sampler.wrap[1] = WrapMode::Clamp;
        descr.sampler.wrap[2] = WrapMode::Clamp;
        descr.sampler.filterMin = FilterMode::Point;
        descr.sampler.filterMag = FilterMode::Point;
        descr.usage = TextureUsage::Sample | TextureUsage::Storage;
        descr.format = TextureFormat::RGBA32UI;
        descr.layers = 2;
        descr.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        m_packedGIDiff = RHI::CreateTexture(descr, "GI.PackedGI.Diff");

        descr.format = TextureFormat::RG32UI;
        m_packedGISpec = RHI::CreateTexture(descr, "GI.PackedGI.Spec");

        descr.samplerType = SamplerType::Sampler2D;
        descr.layers = 1u;
        descr.levels = 1u;
        descr.usage = TextureUsage::Storage;
        descr.format = TextureFormat::RG32UI;
        descr.resolution = { config->InitialWidth, config->InitialHeight, 1u };
        descr.resolution = GetCheckerboardResolution(descr.resolution, m_useCheckerboardTrace);
        m_rayhits = RHI::CreateTexture(descr, "GI.RayHits");

        descr.samplerType = SamplerType::Sampler2DArray;
        descr.layers = 2;
        descr.format = TextureFormat::RGBA32UI;
        m_reservoirs0 = RHI::CreateTexture(descr, "GI.Reservoirs0");
        descr.format = TextureFormat::RG32UI;
        m_reservoirs1 = RHI::CreateTexture(descr, "GI.Reservoirs1");

        descr.layers = 2;
        descr.format = TextureFormat::RGB9E5;
        descr.formatAlias = TextureFormat::R32UI;
        descr.usage = TextureUsage::Storage | TextureUsage::Sample;
        descr.resolution = { config->InitialWidth, config->InitialHeight, 1u };
        m_resolvedGI = RHI::CreateTexture(descr, "GI.Resolved");

        m_voxelizeAttribs.depthStencil.depthCompareOp = Comparison::Off;
        m_voxelizeAttribs.depthStencil.depthWriteEnable = false;
        m_voxelizeAttribs.rasterization.cullMode = CullMode::Off;
        //m_voxelizeAttribs.rasterization.rasterMode = RasterMode::OverEstimate;

        auto hash = HashCache::Get();
        m_parameters = CreateRef<ConstantBuffer>(BufferLayout(
            {
                { ElementType::Float4, hash->pk_GI_VolumeST },
                { ElementType::Uint4, hash->pk_GI_VolumeSwizzle },
                { ElementType::Uint2, hash->pk_GI_RayDither },
                { ElementType::Float, hash->pk_GI_VoxelSize },
                { ElementType::Float, hash->pk_GI_VoxelStepSize },
                { ElementType::Float, hash->pk_GI_VoxelLevelScale },
            }), "GI.Parameters");

        const auto voxelSize = 0.6f;
        const auto angle = PK_FLOAT_PI / 3.0f;
        const auto levelscale = 2.0f * tan(angle / 2.0f) / voxelSize;
        const auto correctionAngle = tan(angle / 8.0f);
        const auto stepSize = (1.0f + correctionAngle) / (1.0f - correctionAngle) * voxelSize / 2.0f;

        m_parameters->Set<float4>(hash->pk_GI_VolumeST, float4(-76.8f, -6.0f, -76.8f, 1.0f / voxelSize));
        m_parameters->Set<float>(hash->pk_GI_VoxelSize, voxelSize);
        m_parameters->Set<float>(hash->pk_GI_VoxelStepSize, stepSize);
        m_parameters->Set<float>(hash->pk_GI_VoxelLevelScale, levelscale);

        RHI::SetBuffer(hash->pk_GI_Parameters, m_parameters->GetRHI());
        RHI::SetImage(hash->pk_GI_VolumeMaskWrite, m_voxelMask.get());
        RHI::SetImage(hash->pk_GI_VolumeWrite, m_voxels.get());
        RHI::SetTexture(hash->pk_GI_VolumeRead, m_voxels.get());
    }

    void PassSceneGI::PreRender(CommandBufferExt cmd, const uint3& resolution)
    {
        auto hash = HashCache::Get();

        uint4 swizzles[3] =
        {
             { 0u, 2u, 1u, 0u },
             { 0u, 1u, 2u, 0u },
             { 1u, 2u, 0u, 0u },
        };

        m_sbtRaytrace.Validate(cmd, m_rayTraceGatherGI);
        m_sbtValidate.Validate(cmd, m_rayTraceValidate);
        m_packedGIDiff->Validate(resolution);
        m_packedGISpec->Validate(resolution);
        m_rayhits->Validate(GetCheckerboardResolution(resolution, m_useCheckerboardTrace));
        m_reservoirs0->Validate(GetCheckerboardResolution(resolution, m_useCheckerboardTrace));
        m_reservoirs1->Validate(GetCheckerboardResolution(resolution, m_useCheckerboardTrace));
        m_resolvedGI->Validate(resolution);

        RHI::SetImage(hash->pk_GI_RayHits, m_rayhits.get());
        RHI::SetImage(hash->pk_Reservoirs0, m_reservoirs0.get());
        RHI::SetImage(hash->pk_Reservoirs1, m_reservoirs1.get());
        RHI::SetImage(hash->pk_GI_PackedDiff, m_packedGIDiff.get());
        RHI::SetImage(hash->pk_GI_PackedSpec, m_packedGISpec.get());
        RHI::SetImage(hash->pk_GI_ResolvedWrite, m_resolvedGI.get());
        RHI::SetTexture(hash->pk_GI_ResolvedRead, m_resolvedGI.get());

        m_rasterAxis = m_frameIndex % 3;
        m_parameters->Set<uint4>(hash->pk_GI_VolumeSwizzle, swizzles[m_rasterAxis]);
        m_parameters->Set<uint2>(hash->pk_GI_RayDither, Math::MurmurHash21(m_frameIndex / 64u));
        m_parameters->FlushBuffer(RHI::GetCommandBuffer(QueueType::Transfer));
        m_frameIndex++;
    }

    void PassSceneGI::PruneVoxels(CommandBufferExt cmd)
    {
        // Clear transparencies every axis cycle
        if (m_rasterAxis == 0)
        {
            cmd->BeginDebugScope("SceneGI.PruneVoxels", PK_COLOR_GREEN);
            RHI::SetImage(HashCache::Get()->pk_Image, m_voxels.get(), 0, 0);
            cmd.Dispatch(m_computeClear, m_voxels->GetResolution());
            cmd->EndDebugScope();
        }
    }

    void PassSceneGI::DispatchRays(CommandBufferExt cmd)
    {
        cmd->BeginDebugScope("SceneGI.DispatchRays", PK_COLOR_GREEN);
        cmd.SetShaderBindingTable(&m_sbtRaytrace);
        cmd.DispatchRays(m_rayTraceGatherGI, m_rayhits->GetResolution());
        cmd->EndDebugScope();
    }

    void PassSceneGI::ReprojectGI(CommandBufferExt cmd)
    {
        cmd->BeginDebugScope("SceneGI.Reproject", PK_COLOR_GREEN);
        auto resolution = m_packedGIDiff->GetResolution();
        cmd.Dispatch(m_computeReproject, { resolution.x, resolution.y, 1u });
        cmd->EndDebugScope();
    }

    void PassSceneGI::Voxelize(CommandBufferExt cmd, IBatcher* batcher, uint32_t batchGroup)
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
        cmd->SetRenderTarget({ viewports[m_rasterAxis].z, viewports[m_rasterAxis].w, 1 });
        cmd.SetViewPort(viewports[m_rasterAxis]);
        cmd.SetScissor(viewports[m_rasterAxis]);
        batcher->RenderGroup(cmd, batchGroup, &m_voxelizeAttribs, hash->PK_META_PASS_GIVOXELIZE);

        cmd->EndDebugScope();
    }

    void PassSceneGI::RenderGI(CommandBufferExt cmd)
    {
        auto resolution = m_packedGIDiff->GetResolution();
        uint3 dimension = { resolution.x, resolution.y, 1u };
        uint3 chbdimension = GetCheckerboardResolution(dimension, m_useCheckerboardTrace);
        cmd->BeginDebugScope("SceneGI.Filter", PK_COLOR_GREEN);
        cmd.Dispatch(m_computeShadeHits, chbdimension);
        cmd.Dispatch(m_computeAccumulate, chbdimension);
        cmd.Dispatch(m_computePostFilter, m_useCheckerboardTrace ? 1 : 0, chbdimension);
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

    void PassSceneGI::ValidateReservoirs(CommandBufferExt cmd)
    {
        if (m_useReSTIR)
        {
            cmd->BeginDebugScope("SceneGI.ValidateReservoirs", PK_COLOR_GREEN);
            cmd.SetShaderBindingTable(&m_sbtValidate);
            cmd.DispatchRays(m_rayTraceValidate, m_reservoirs0->GetResolution());
            cmd->EndDebugScope();
        }
    }

    void PassSceneGI::OnUpdateParameters(const RendererConfig* config)
    {
        m_useCheckerboardTrace = config->GICheckerboardTrace;
        m_useReSTIR = config->GIReSTIR;
        RHI::SetKeyword("PK_GI_CHECKERBOARD_TRACE", m_useCheckerboardTrace);
        RHI::SetKeyword("PK_GI_SPEC_VIRT_REPROJECT", config->GISpecularVirtualReproject);
        RHI::SetKeyword("PK_GI_SSRT_PRETRACE", config->GIScreenSpacePretrace);
        RHI::SetKeyword("PK_GI_RESTIR", config->GIReSTIR);
    }
}