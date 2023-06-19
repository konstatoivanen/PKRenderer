#include "PrecompiledHeader.h"
#include "PassSceneGI.h"
#include "Rendering/HashCache.h"
#include "Math/FunctionsMisc.h"

namespace PK::Rendering::Passes
{
    using namespace Core;
    using namespace Core::Services;
    using namespace Math;
    using namespace Utilities;
    using namespace Structs;
    using namespace Objects;

    PassSceneGI::PassSceneGI(AssetDatabase* assetDatabase, const ApplicationConfig* config)
    {
        m_computeClear = assetDatabase->Find<Shader>("CS_GI_Clear");
        m_computeMipmap = assetDatabase->Find<Shader>("CS_GI_VolumeMipmap");
        m_computeAccumulate = assetDatabase->Find<Shader>("CS_GI_Accumulate");
        m_computeReproject = assetDatabase->Find<Shader>("CS_GI_Reproject");
        m_computeScreenMip = assetDatabase->Find<Shader>("CS_GI_ScreenMip");
        m_computeHistoryFill = assetDatabase->Find<Shader>("CS_GI_HistoryFill");
        m_computeDiskFilter = assetDatabase->Find<Shader>("CS_GI_DiskFilter");
        m_rayTraceGatherGI = assetDatabase->Find<Shader>("RS_GI_Raytrace");

        TextureDescriptor descr{};
        descr.samplerType = SamplerType::Sampler3D;
        descr.format = TextureFormat::RGBA16F;
        descr.sampler.filterMin = FilterMode::Trilinear;
        descr.sampler.filterMag = FilterMode::Trilinear;
        descr.sampler.wrap[0] = WrapMode::Border;
        descr.sampler.wrap[1] = WrapMode::Border;
        descr.sampler.wrap[2] = WrapMode::Border;
        descr.sampler.borderColor = BorderColor::FloatClear;
        descr.sampler.mipMax = 6.0f;
        descr.resolution = { 256u, 128u, 256u };
        descr.levels = 7u;
        descr.usage = TextureUsage::Sample | TextureUsage::Storage;
        m_voxels = Texture::Create(descr, "GI.VoxelVolume");

        descr.format = TextureFormat::R8UI;
        descr.sampler.borderColor = BorderColor::IntClear;
        descr.levels = 1u;
        descr.sampler.mipMax = 0.0f;
        m_voxelMask = Texture::Create(descr, "GI.VoxelVolumeMask");

        descr.samplerType = SamplerType::Sampler2DArray;
        descr.sampler.wrap[0] = WrapMode::Clamp;
        descr.sampler.wrap[1] = WrapMode::Clamp;
        descr.sampler.wrap[2] = WrapMode::Clamp;
        descr.sampler.filterMin = FilterMode::Point;
        descr.sampler.filterMag = FilterMode::Point;
        descr.usage = TextureUsage::Sample | TextureUsage::Storage;
        descr.format = TextureFormat::RG32UI;
        descr.layers = 8;
        descr.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        m_screenData = Texture::Create(descr, "GI.ScreenData");

        descr.layers = 3u;
        descr.levels = 4u;
        descr.resolution = { config->InitialWidth / 2u, config->InitialHeight / 2u, 1u };
        m_screenDataMips = Texture::Create(descr, "GI.ScreenDataMips");

        descr.samplerType = SamplerType::Sampler2D;
        descr.levels = 1u;
        descr.layers = 1u;
        descr.usage = TextureUsage::Storage;
        descr.format = TextureFormat::R32UI;
        m_rayhits = Texture::Create(descr, "GI.RayHits");

        m_voxelizeAttribs.depthStencil.depthCompareOp = Comparison::Off;
        m_voxelizeAttribs.depthStencil.depthWriteEnable = false;
        m_voxelizeAttribs.rasterization.cullMode = CullMode::Off;
        //m_voxelizeAttribs.rasterization.rasterMode = RasterMode::OverEstimate;

        auto hash = HashCache::Get();
        m_parameters = CreateRef<ConstantBuffer>(BufferLayout(
        {
            { ElementType::Float4, hash->pk_GI_VolumeST },
            { ElementType::Uint4, hash->pk_GI_VolumeSwizzle },
            { ElementType::Int4, hash->pk_GI_Checkerboard_Offset },
            { ElementType::Uint2, hash->pk_GI_RayDither },
            { ElementType::Float, hash->pk_GI_VoxelSize },
            { ElementType::Float, hash->pk_GI_ChromaBias },
        }), "GI.Parameters");

        m_parameters->Set<float4>(hash->pk_GI_VolumeST, float4(-76.8f, -6.0f, -76.8f, 1.0f / 0.6f));
        m_parameters->Set<float>(hash->pk_GI_VoxelSize, 0.6f);
        m_parameters->Set<float>(hash->pk_GI_ChromaBias, 0.1f);

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

        m_shaderBindingTable.Validate(
            GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Transfer),
            GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Compute),
            m_rayTraceGatherGI);

        m_screenData->Validate(resolution);
        m_rayhits->Validate(resolution);
        m_screenDataMips->Validate({ resolution.x / 2u, resolution.y / 2u, resolution.z });

        GraphicsAPI::SetImage(hash->pk_GI_RayHits, m_rayhits.get());
        GraphicsAPI::SetTexture(hash->pk_GI_ScreenDataMips, m_screenDataMips.get());

        m_rasterAxis = m_frameIndex % 3;
        auto checkerboardIndex = m_frameIndex % 4;

        m_parameters->Set<uint4>(hash->pk_GI_VolumeSwizzle, swizzles[m_rasterAxis]);
        m_parameters->Set<int4>(hash->pk_GI_Checkerboard_Offset, { (m_frameIndex % 4) / 2, (m_frameIndex % 4) % 2, 0, 0 });
        m_parameters->Set<uint2>(hash->pk_GI_RayDither, Functions::MurmurHash21(m_frameIndex / 64u));
        m_parameters->FlushBuffer(QueueType::Transfer);
        m_frameIndex++;
    }

    void PassSceneGI::PruneVoxels(Objects::CommandBuffer* cmd)
    {
        // Clear transparencies every axis cycle
        if (m_rasterAxis == 0)
        {
            cmd->BeginDebugScope("SceneGI.PruneVoxels", PK_COLOR_GREEN);
            GraphicsAPI::SetImage(HashCache::Get()->_DestinationTex, m_voxels.get(), 0, 0);
            cmd->Dispatch(m_computeClear, m_voxels->GetResolution());
            cmd->EndDebugScope();
        }
    }

    void PassSceneGI::DispatchRays(Objects::CommandBuffer* cmd)
    {
        cmd->BeginDebugScope("SceneGI.DispatchRays", PK_COLOR_GREEN);
        auto resolution = m_rayhits->GetResolution();
        cmd->DispatchRays(m_rayTraceGatherGI, { resolution.x, resolution.y, 1 });
        cmd->EndDebugScope();
    }

    void PassSceneGI::RenderVoxels(CommandBuffer* cmd, Batcher* batcher, uint32_t batchGroup)
    {
        cmd->BeginDebugScope("SceneGI.Preprocess", PK_COLOR_GREEN);

        auto hash = HashCache::Get();
        auto resolution = m_screenData->GetResolution();
        auto volres = m_voxels->GetResolution();

        uint4 viewports[3] =
        {
            {0u, 0u, volres.x, volres.z },
            {0u, 0u, volres.x, volres.y },
            {0u, 0u, volres.y, volres.z },
        };

        GraphicsAPI::SetTexture(hash->pk_GI_ScreenDataRead, m_screenData.get(), { 0,4,0,4 });
        GraphicsAPI::SetImage(hash->pk_GI_ScreenDataWrite, m_screenData.get(), { 0,0,0,4 });
        cmd->Dispatch(m_computeReproject, 0, { resolution.x, resolution.y, 1u });

        cmd->SetRenderTarget({ viewports[m_rasterAxis].z, viewports[m_rasterAxis].w, 1 });
        cmd->SetViewPort(viewports[m_rasterAxis]);
        cmd->SetScissor(viewports[m_rasterAxis]);
        batcher->Render(cmd, batchGroup, &m_voxelizeAttribs, hash->PK_META_PASS_GIVOXELIZE);

        GraphicsAPI::SetTexture(hash->_SourceTex, m_voxels.get());
        GraphicsAPI::SetImage(hash->_DestinationTex, m_voxels.get(), 1, 0);
        GraphicsAPI::SetImage(hash->_DestinationMip1, m_voxels.get(), 2, 0);
        GraphicsAPI::SetImage(hash->_DestinationMip2, m_voxels.get(), 3, 0);
        cmd->Dispatch(m_computeMipmap, 0, volres >> 1u);
        GraphicsAPI::SetImage(hash->_DestinationTex, m_voxels.get(), 4, 0);
        GraphicsAPI::SetImage(hash->_DestinationMip1, m_voxels.get(), 5, 0);
        GraphicsAPI::SetImage(hash->_DestinationMip2, m_voxels.get(), 6, 0);
        cmd->Dispatch(m_computeMipmap, 0, volres >> 4u);

        cmd->EndDebugScope();
    }

    void PassSceneGI::RenderGI(CommandBuffer* cmd)
    {
        auto hash = HashCache::Get();
        auto range0 = TextureViewRange(0, 0, 0, 4);
        auto range1 = TextureViewRange(0, 4, 0, 4);

        auto resolution = m_screenData->GetResolution();
        uint3 dimension = { resolution.x, resolution.y, 1u };

        cmd->BeginDebugScope("SceneGI.Filter", PK_COLOR_GREEN);

        GraphicsAPI::SetTexture(hash->pk_GI_ScreenDataRead, m_screenData.get(), range0);
        GraphicsAPI::SetImage(hash->pk_GI_ScreenDataWrite, m_screenData.get(), range0);
        cmd->Dispatch(m_computeAccumulate, 0, dimension);

        GraphicsAPI::SetImage(hash->_DestinationMip1, m_screenDataMips.get(), { 0, 0, 1, 3 });
        GraphicsAPI::SetImage(hash->_DestinationMip2, m_screenDataMips.get(), { 1, 0, 1, 3 });
        GraphicsAPI::SetImage(hash->_DestinationMip3, m_screenDataMips.get(), { 2, 0, 1, 3 });
        GraphicsAPI::SetImage(hash->_DestinationMip4, m_screenDataMips.get(), { 3, 0, 1, 3 });
        cmd->Dispatch(m_computeScreenMip, 0, m_screenDataMips->GetResolution());

        cmd->Dispatch(m_computeHistoryFill, 0, dimension);

        GraphicsAPI::SetImage(hash->pk_GI_ScreenDataWrite, m_screenData.get(), range1);
        cmd->Dispatch(m_computeDiskFilter, 0, dimension);

        GraphicsAPI::SetTexture(hash->pk_GI_ScreenDataRead, m_screenData.get(), range1);
        cmd->EndDebugScope();
    }
}