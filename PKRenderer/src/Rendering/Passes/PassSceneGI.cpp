#include "PrecompiledHeader.h"
#include "PassSceneGI.h"
#include "Rendering/HashCache.h"

namespace PK::Rendering::Passes
{
    using namespace Core;
    using namespace Core::Services;
    using namespace Math;
    using namespace Utilities;
    using namespace Structs;
    using namespace Objects;

    static uint3 GetScreenDataMipResolution(uint w, uint h)
    {
        return uint3(8u * uint(ceil(w / 16.0f)), 8u * uint(ceil(h / 16.0f)), 1u);
    }

    PassSceneGI::PassSceneGI(AssetDatabase* assetDatabase, const ApplicationConfig* config)
    {
        m_computeClear = assetDatabase->Find<Shader>("CS_GI_Clear");
        m_computeMipmap = assetDatabase->Find<Shader>("CS_GI_VolumeMipmap");
        m_computeAccumulate = assetDatabase->Find<Shader>("CS_GI_Accumulate");
        m_computeReproject = assetDatabase->Find<Shader>("CS_GI_Reproject");
        m_computeScreenMip = assetDatabase->Find<Shader>("CS_GI_ScreenMip");
        m_computeDiffuseHistoryFill = assetDatabase->Find<Shader>("CS_GI_DiffuseHistoryFill");
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
        descr.resolution = GetScreenDataMipResolution(config->InitialWidth, config->InitialHeight);
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
        auto mipResolution = GetScreenDataMipResolution(resolution.x, resolution.y);

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
        m_screenDataMips->Validate(mipResolution);

        GraphicsAPI::SetImage(hash->pk_GI_RayHits, m_rayhits.get());
        GraphicsAPI::SetImage(hash->pk_GI_ScreenDataMip1, m_screenDataMips.get(), { 0, 0, 1, 3 });
        GraphicsAPI::SetImage(hash->pk_GI_ScreenDataMip2, m_screenDataMips.get(), { 1, 0, 1, 3 });
        GraphicsAPI::SetImage(hash->pk_GI_ScreenDataMip3, m_screenDataMips.get(), { 2, 0, 1, 3 });
        GraphicsAPI::SetImage(hash->pk_GI_ScreenDataMip4, m_screenDataMips.get(), { 3, 0, 1, 3 });
        GraphicsAPI::SetTexture(hash->pk_GI_ScreenDataMips, m_screenDataMips.get());

        m_rasterAxis = (m_rasterAxis + 1) % 3;
        m_checkerboardIndex = (m_checkerboardIndex + 1) % 4;
        m_parameters->Set<uint4>(hash->pk_GI_VolumeSwizzle, swizzles[m_rasterAxis]);
        m_parameters->Set<int4>(hash->pk_GI_Checkerboard_Offset, { m_checkerboardIndex / 2, m_checkerboardIndex % 2, 0, 0 });
        m_parameters->FlushBuffer(QueueType::Transfer);
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

        auto range0 = TextureViewRange(0, 0, 0, 4);
        auto range1 = TextureViewRange(0, 4, 0, 4);
        GraphicsAPI::SetTexture(hash->pk_GI_ScreenDataRead, m_screenData.get(), range1);
        GraphicsAPI::SetImage(hash->pk_GI_ScreenDataWrite, m_screenData.get(), range0);
        cmd->Dispatch(m_computeReproject, 0, { resolution.x, resolution.y, 1u });

        cmd->SetRenderTarget({ viewports[m_rasterAxis].z, viewports[m_rasterAxis].w, 1 });
        cmd->SetViewPort(viewports[m_rasterAxis]);
        cmd->SetScissor(viewports[m_rasterAxis]);
        batcher->Render(cmd, batchGroup, &m_voxelizeAttribs, hash->PK_META_PASS_GIVOXELIZE);

        GraphicsAPI::SetTexture(hash->_SourceTex, m_voxels.get());

        for (auto i = 1u; i < m_voxels->GetLevels(); ++i)
        {
            GraphicsAPI::SetImage(hash->_DestinationTex, m_voxels.get(), i, 0);
            cmd->Dispatch(m_computeMipmap, 0, { volres.x >> i, volres.y >> i, volres.z >> i });
        }

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
        cmd->Dispatch(m_computeScreenMip, 0, m_screenDataMips->GetResolution());
        cmd->Dispatch(m_computeDiffuseHistoryFill, 0, dimension);

        GraphicsAPI::SetImage(hash->pk_GI_ScreenDataWrite, m_screenData.get(), range1);
        cmd->Dispatch(m_computeDiskFilter, 0, dimension);

        GraphicsAPI::SetTexture(hash->pk_GI_ScreenDataRead, m_screenData.get(), range1);
        cmd->EndDebugScope();
    }
}