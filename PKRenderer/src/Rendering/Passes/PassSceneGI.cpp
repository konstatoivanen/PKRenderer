#include "PrecompiledHeader.h"
#include "Math/FunctionsMisc.h"
#include "Rendering/HashCache.h"
#include "PassSceneGI.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::Services;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    uint3 GetCheckerboardResolution(const uint3& resolution, bool halfRes)
    {
        return { resolution.x / (halfRes ? 2 : 1), resolution.y, resolution.z };
    }

    PassSceneGI::PassSceneGI(AssetDatabase* assetDatabase, const ApplicationConfig* config)
    {
        PK_LOG_VERBOSE("Initializing Scene GI");
        PK_LOG_SCOPE_INDENT(local);

        m_computeClear = assetDatabase->Find<Shader>("CS_GI_Clear");
        m_computeMipmap = assetDatabase->Find<Shader>("CS_GI_VolumeMipmap");
        m_computeAccumulate = assetDatabase->Find<Shader>("CS_GI_Accumulate");
        m_computeShadeHits = assetDatabase->Find<Shader>("CS_GI_ShadeHits");
        m_computeReproject = assetDatabase->Find<Shader>("CS_GI_Reproject");
        m_computeGradients = assetDatabase->Find<Shader>("CS_GI_GradientEstimation");
        m_computePostFilter = assetDatabase->Find<Shader>("CS_GI_PostFilter");
        m_rayTraceGatherGI = assetDatabase->Find<Shader>("RS_GI_Raytrace");
        m_rayTraceValidate = assetDatabase->Find<Shader>("RS_GI_ValidateReservoirs");
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
        m_voxels = Texture::Create(descr, "GI.VoxelVolume");

        descr.format = TextureFormat::R8UI;
        descr.sampler.borderColor = BorderColor::IntClear;
        descr.levels = 1u;
        m_voxelMask = Texture::Create(descr, "GI.VoxelVolumeMask");

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
        m_packedGIDiff = Texture::Create(descr, "GI.PackedGI.Diff");

        descr.format = TextureFormat::RG32UI;
        m_packedGISpec = Texture::Create(descr, "GI.PackedGI.Spec");

        descr.samplerType = SamplerType::Sampler2D;
        descr.layers = 1u;
        descr.levels = 1u;
        descr.usage = TextureUsage::Storage;
        descr.format = TextureFormat::RG32UI;
        descr.resolution = { config->InitialWidth, config->InitialHeight, 1u };
        descr.resolution = GetCheckerboardResolution(descr.resolution, m_useCheckerboardTrace);
        m_rayhits = Texture::Create(descr, "GI.RayHits");

        descr.samplerType = SamplerType::Sampler2DArray;
        descr.layers = 2;
        descr.format = TextureFormat::RGBA32UI;
        m_reservoirs0 = Texture::Create(descr, "GI.Reservoirs0");
        descr.format = TextureFormat::RG32UI;
        m_reservoirs1 = Texture::Create(descr, "GI.Reservoirs1");

        descr.layers = 2;
        descr.format = TextureFormat::RGB9E5;
        descr.formatAlias = TextureFormat::R32UI;
        descr.usage = TextureUsage::Storage | TextureUsage::Sample;
        descr.resolution = { config->InitialWidth, config->InitialHeight, 1u };
        m_resolvedGI = Texture::Create(descr, "GI.Resolved");

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
        }), "GI.Parameters");

        m_parameters->Set<float4>(hash->pk_GI_VolumeST, float4(-76.8f, -6.0f, -76.8f, 1.0f / 0.6f));
        m_parameters->Set<float>(hash->pk_GI_VoxelSize, 0.6f);

        GraphicsAPI::SetBuffer(hash->pk_GI_Parameters, m_parameters->GetBuffer());
        GraphicsAPI::SetImage(hash->pk_GI_VolumeMaskWrite, m_voxelMask.get());
        GraphicsAPI::SetImage(hash->pk_GI_VolumeWrite, m_voxels.get());
        GraphicsAPI::SetTexture(hash->pk_GI_VolumeRead, m_voxels.get());
    }

    void PassSceneGI::PreRender(CommandBuffer* cmd, const uint3& resolution)
    {
        auto hash = HashCache::Get();

        uint4 swizzles[3] =
        {
             { 0u, 2u, 1u, 0u },
             { 0u, 1u, 2u, 0u },
             { 1u, 2u, 0u, 0u },
        };

        auto cmdTransfer = GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Transfer);
        m_sbtRaytrace.Validate(cmdTransfer, m_rayTraceGatherGI);
        m_sbtValidate.Validate(cmdTransfer, m_rayTraceValidate);

        m_packedGIDiff->Validate(resolution);
        m_packedGISpec->Validate(resolution);
        m_rayhits->Validate(GetCheckerboardResolution(resolution, m_useCheckerboardTrace));
        m_reservoirs0->Validate(GetCheckerboardResolution(resolution, m_useCheckerboardTrace));
        m_reservoirs1->Validate(GetCheckerboardResolution(resolution, m_useCheckerboardTrace));
        m_resolvedGI->Validate(resolution);

        GraphicsAPI::SetImage(hash->pk_GI_RayHits, m_rayhits.get());
        GraphicsAPI::SetImage(hash->pk_Reservoirs0, m_reservoirs0.get());
        GraphicsAPI::SetImage(hash->pk_Reservoirs1, m_reservoirs1.get());
        GraphicsAPI::SetImage(hash->pk_GI_PackedDiff, m_packedGIDiff.get());
        GraphicsAPI::SetImage(hash->pk_GI_PackedSpec, m_packedGISpec.get());
        GraphicsAPI::SetImage(hash->pk_GI_ResolvedWrite, m_resolvedGI.get());
        GraphicsAPI::SetTexture(hash->pk_GI_ResolvedRead, m_resolvedGI.get());

        m_rasterAxis = m_frameIndex % 3;
        m_parameters->Set<uint4>(hash->pk_GI_VolumeSwizzle, swizzles[m_rasterAxis]);
        m_parameters->Set<uint2>(hash->pk_GI_RayDither, Functions::MurmurHash21(m_frameIndex / 64u));
        m_parameters->FlushBuffer(QueueType::Transfer);
        m_frameIndex++;
    }

    void PassSceneGI::PruneVoxels(CommandBuffer* cmd)
    {
        // Clear transparencies every axis cycle
        if (m_rasterAxis == 0)
        {
            cmd->BeginDebugScope("SceneGI.PruneVoxels", PK_COLOR_GREEN);
            GraphicsAPI::SetImage(HashCache::Get()->pk_Image, m_voxels.get(), 0, 0);
            cmd->Dispatch(m_computeClear, m_voxels->GetResolution());
            cmd->EndDebugScope();
        }
    }

    void PassSceneGI::DispatchRays(CommandBuffer* cmd)
    {
        cmd->BeginDebugScope("SceneGI.DispatchRays", PK_COLOR_GREEN);
        m_sbtRaytrace.Bind(cmd);
        cmd->DispatchRays(m_rayTraceGatherGI, m_rayhits->GetResolution());
        cmd->EndDebugScope();
    }

    void PassSceneGI::ReprojectGI(CommandBuffer* cmd)
    {
        cmd->BeginDebugScope("SceneGI.Reproject", PK_COLOR_GREEN);
        auto resolution = m_packedGIDiff->GetResolution();
        cmd->Dispatch(m_computeReproject, { resolution.x, resolution.y, 1u });
        cmd->EndDebugScope();
    }

    void PassSceneGI::Voxelize(CommandBuffer* cmd, Batcher* batcher, uint32_t batchGroup)
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
        cmd->SetViewPort(viewports[m_rasterAxis]);
        cmd->SetScissor(viewports[m_rasterAxis]);
        batcher->Render(cmd, batchGroup, &m_voxelizeAttribs, hash->PK_META_PASS_GIVOXELIZE);

        cmd->EndDebugScope();
    }

    void PassSceneGI::RenderGI(CommandBuffer* cmd)
    {
        auto hash = HashCache::Get();

        auto resolution = m_packedGIDiff->GetResolution();
        uint3 dimension = { resolution.x, resolution.y, 1u };
        uint3 chbdimension = GetCheckerboardResolution(dimension, m_useCheckerboardTrace);

        cmd->BeginDebugScope("SceneGI.Filter", PK_COLOR_GREEN);

        cmd->Dispatch(m_computeShadeHits, chbdimension);
        cmd->Dispatch(m_computeAccumulate, chbdimension);
        cmd->Dispatch(m_computePostFilter, m_useCheckerboardTrace ? 1 : 0, chbdimension);

        cmd->EndDebugScope();
    }

    void PassSceneGI::VoxelMips(CommandBuffer* cmd)
    {
        // Voxel mips
        auto hash = HashCache::Get();
        auto volumesize = m_voxels->GetResolution();
        GraphicsAPI::SetTexture(hash->pk_Texture, m_voxels.get());
        GraphicsAPI::SetImage(hash->pk_Image, m_voxels.get(), 1, 0);
        GraphicsAPI::SetImage(hash->pk_Image1, m_voxels.get(), 2, 0);
        GraphicsAPI::SetImage(hash->pk_Image2, m_voxels.get(), 3, 0);
        cmd->Dispatch(m_computeMipmap, 0, volumesize >> 1u);
        GraphicsAPI::SetImage(hash->pk_Image, m_voxels.get(), 4, 0);
        GraphicsAPI::SetImage(hash->pk_Image1, m_voxels.get(), 5, 0);
        GraphicsAPI::SetImage(hash->pk_Image2, m_voxels.get(), 6, 0);
        cmd->Dispatch(m_computeMipmap, 0, volumesize >> 4u);
    }

    void PassSceneGI::ValidateReservoirs(CommandBuffer* cmd)
    {
        if (m_useReSTIR)
        {
            cmd->BeginDebugScope("SceneGI.ValidateReservoirs", PK_COLOR_GREEN);
            m_sbtValidate.Bind(cmd);
            cmd->DispatchRays(m_rayTraceValidate, m_reservoirs0->GetResolution());
            cmd->EndDebugScope();
        }
    }

    void PassSceneGI::OnUpdateParameters(const ApplicationConfig* config)
    {
        m_useCheckerboardTrace = config->GICheckerboardTrace;
        m_useReSTIR = config->GIReSTIR;
        GraphicsAPI::SetKeyword("PK_GI_CHECKERBOARD_TRACE", m_useCheckerboardTrace);
        GraphicsAPI::SetKeyword("PK_GI_SPEC_VIRT_REPROJECT", config->GISpecularVirtualReproject);
        GraphicsAPI::SetKeyword("PK_GI_SSRT_PRETRACE", config->GIScreenSpacePretrace);
        GraphicsAPI::SetKeyword("PK_GI_RESTIR", config->GIReSTIR);
    }
}