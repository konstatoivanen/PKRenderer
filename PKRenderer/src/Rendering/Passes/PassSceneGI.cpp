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

    PassSceneGI::PassSceneGI(AssetDatabase* assetDatabase, const ApplicationConfig* config)
    {
        m_computeClear = assetDatabase->Find<Shader>("CS_SceneGI_Clear");
        m_computeMipmap = assetDatabase->Find<Shader>("CS_SceneGI_Mipmap");
        m_computeBakeGI = assetDatabase->Find<Shader>("CS_SceneGI_Bake");
        m_computeReprojectMask = assetDatabase->Find<Shader>("CS_SceneGI_ReprojectMask");
        m_computeDenoise = assetDatabase->Find<Shader>("CS_SceneGI_Denoise");
        m_rayTraceGatherGI = assetDatabase->Find<Shader>("RS_SceneGI_Gather");

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

        descr.samplerType = SamplerType::Sampler2D;
        descr.format = TextureFormat::R8UI;
        descr.layers = 1u;
        descr.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        m_screenSpaceMask = Texture::Create(descr, "GI.ScreenSpaceMaskTexture");

        descr.samplerType = SamplerType::Sampler2D;
        descr.sampler.filterMin = FilterMode::Bilinear;
        descr.sampler.filterMag = FilterMode::Bilinear;
        descr.sampler.wrap[0] = WrapMode::Clamp;
        descr.sampler.wrap[1] = WrapMode::Clamp;
        descr.sampler.wrap[2] = WrapMode::Clamp;
        descr.usage = TextureUsage::Storage;
        descr.format = TextureFormat::RG16F;
        descr.levels = 1u;
        descr.layers = 1;
        m_screenSpaceRayhits = Texture::Create(descr, "GI.RayHits");

        descr.samplerType = SamplerType::Sampler2DArray;
        descr.usage = TextureUsage::DefaultStorage;
        descr.layers = 4;
        descr.format = TextureFormat::RGBA16F;
        m_screenSpaceSHY = Texture::Create(descr, "GI.ScreenSpaceSHY");
        descr.format = TextureFormat::RG16F;
        m_screenSpaceCoCg = Texture::Create(descr, "GI.ScreenSpaceCoCg");

        descr.samplerType = SamplerType::Sampler2D;
        descr.layers = 1u;
        descr.format = TextureFormat::RGBA16F;
        m_previousForwardOutput = Texture::Create(descr, "GI.ForwardCache");

        descr.format = TextureFormat::R16F;
        m_screenSpaceAO = Texture::Create(descr, "GI.AO");

        m_voxelizeAttribs.depthStencil.depthCompareOp = Comparison::Off;
        m_voxelizeAttribs.depthStencil.depthWriteEnable = false;
        m_voxelizeAttribs.rasterization.cullMode = CullMode::Off;
        //m_voxelizeAttribs.rasterization.rasterMode = RasterMode::OverEstimate;

        auto hash = HashCache::Get();
        m_parameters = CreateRef<ConstantBuffer>(BufferLayout(
        {
            { ElementType::Float4, hash->pk_SceneGI_ST },
            { ElementType::Uint4, hash->pk_SceneGI_Swizzle },
            { ElementType::Int4, hash->pk_SceneGI_Checkerboard_Offset },
            { ElementType::Uint, hash->pk_SceneGI_SampleIndex },
            { ElementType::Uint, hash->pk_SceneGI_SampleCount },
            { ElementType::Float, hash->pk_SceneGI_VoxelSize },
            { ElementType::Float, hash->pk_SceneGI_LuminanceGain },
            { ElementType::Float, hash->pk_SceneGI_ChrominanceGain },
        }), "GI.Parameters");

        m_parameters->Set<float4>(hash->pk_SceneGI_ST, float4(-76.8f, -6.0f, -76.8f, 1.0f / 0.6f));
        m_parameters->Set<float>(hash->pk_SceneGI_VoxelSize, 0.6f);
        m_parameters->Set<float>(hash->pk_SceneGI_LuminanceGain, 1.0f);
        m_parameters->Set<float>(hash->pk_SceneGI_ChrominanceGain, 3.14f);
        m_parameters->Set<uint>(hash->pk_SceneGI_SampleIndex, 0u);
        m_parameters->Set<uint>(hash->pk_SceneGI_SampleCount, 256u);

        GraphicsAPI::SetBuffer(hash->pk_SceneGI_Params, m_parameters->GetBuffer());
        GraphicsAPI::SetImage(hash->pk_SceneGI_VolumeMaskWrite, m_voxelMask.get());
        GraphicsAPI::SetImage(hash->pk_SceneGI_VolumeWrite, m_voxels.get());
        GraphicsAPI::SetTexture(hash->pk_SceneGI_VolumeRead, m_voxels.get());
        GraphicsAPI::SetImage(hash->pk_ScreenGI_Mask, m_screenSpaceMask.get());
        GraphicsAPI::SetImage(hash->pk_ScreenGI_Hits, m_screenSpaceRayhits.get());
        GraphicsAPI::SetTexture(hash->pk_ScreenGI_ForwardOuput, m_previousForwardOutput.get());
        GraphicsAPI::SetImage(hash->pk_ScreenGI_AO, m_screenSpaceAO.get());

        auto tableInfo = m_rayTraceGatherGI->GetShaderBindingTableInfo();
        auto tableUintCount = tableInfo.totalTableSize / sizeof(uint32_t);
        m_shaderBindingTable = Buffer::Create(ElementType::Uint, tableUintCount, BufferUsage::DefaultShaderBindingTable, "GI.ShaderBindingTable");
        GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Transfer)->UploadBufferData(m_shaderBindingTable.get(), tableInfo.handleData);

        auto cmd = GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Compute);
        cmd->SetShaderBindingTable(RayTracingShaderGroup::RayGeneration, 
            m_shaderBindingTable.get(), 
            tableInfo.byteOffsets[(uint32_t)RayTracingShaderGroup::RayGeneration], 
            tableInfo.handleSizeAligned,
            tableInfo.handleSizeAligned);
        
        cmd->SetShaderBindingTable(RayTracingShaderGroup::Miss,
            m_shaderBindingTable.get(),
            tableInfo.byteOffsets[(uint32_t)RayTracingShaderGroup::Miss],
            tableInfo.handleSizeAligned,
            tableInfo.handleSizeAligned);
        
        cmd->SetShaderBindingTable(RayTracingShaderGroup::Hit,
            m_shaderBindingTable.get(),
            tableInfo.byteOffsets[(uint32_t)RayTracingShaderGroup::Hit],
            tableInfo.handleSizeAligned,
            tableInfo.handleSizeAligned);
    }

    void PassSceneGI::PreRender(CommandBuffer* cmd, const uint3& resolution)
    {
        auto hash = HashCache::Get();

        if (m_screenSpaceRayhits->Validate(resolution))
        {
            GraphicsAPI::SetImage(hash->pk_ScreenGI_Hits, m_screenSpaceRayhits.get());
        }

        if (m_screenSpaceMask->Validate(resolution))
        {
            GraphicsAPI::SetImage(hash->pk_ScreenGI_Mask, m_screenSpaceMask.get());
        }
        
        if (m_previousForwardOutput->Validate(resolution))
        {
            GraphicsAPI::SetTexture(hash->pk_ScreenGI_ForwardOuput, m_previousForwardOutput.get());
        }

        if (m_screenSpaceAO->Validate(resolution))
        {
            GraphicsAPI::SetImage(hash->pk_ScreenGI_AO, m_screenSpaceAO.get());
        }

        m_screenSpaceSHY->Validate(resolution);
        m_screenSpaceCoCg->Validate(resolution);
        
        uint4 swizzles[3] =
        {
             { 0u, 2u, 1u, 0u },
             { 0u, 1u, 2u, 0u },
             { 1u, 2u, 0u, 0u },
        };

        m_rasterAxis = (m_rasterAxis + 1) % 3;
        m_checkerboardIndex = (m_checkerboardIndex + 1) % 4;

        m_parameters->Set<uint4>(hash->pk_SceneGI_Swizzle, swizzles[m_rasterAxis]);
        m_parameters->Set<int4>(hash->pk_SceneGI_Checkerboard_Offset, { m_checkerboardIndex / 2, m_checkerboardIndex % 2, 0, 0 });
        m_parameters->Set<uint>(hash->pk_SceneGI_SampleIndex, m_rayIndex++ % 256u);

        m_parameters->FlushBuffer(QueueType::Transfer);
    }

    void PassSceneGI::PruneVoxels(Objects::CommandBuffer* cmd)
    {
        // Clear transparencies every axis cycle
        if (m_rasterAxis == 0)
        {
            cmd->BeginDebugScope("SceneGI.PruneVoxels", PK_COLOR_GREEN);
            auto volres = m_voxels->GetResolution();
            GraphicsAPI::SetImage(HashCache::Get()->_DestinationTex, m_voxels.get(), 0, 0);
            cmd->Dispatch(m_computeClear, { volres.x / 8u, volres.y / 8u, volres.z / 8u });
            cmd->EndDebugScope();
        }
    }

    void PassSceneGI::DispatchRays(Objects::CommandBuffer* cmd)
    {
        cmd->BeginDebugScope("SceneGI.DispatchRays", PK_COLOR_GREEN);
        auto resolution = m_screenSpaceRayhits->GetResolution();
        cmd->DispatchRays(m_rayTraceGatherGI, { resolution.x, resolution.y, 1 });
        cmd->EndDebugScope();
    }

    void PassSceneGI::RenderVoxels(CommandBuffer* cmd, Batcher* batcher, uint32_t batchGroup)
    {
        cmd->BeginDebugScope("SceneGI.ReprojectMask", PK_COLOR_GREEN);

        auto hash = HashCache::Get();
        auto resolution = m_screenSpaceSHY->GetResolution();
        uint3 groupSize = { (uint)ceil(resolution.x / 16.0f), (uint)ceil(resolution.y / 16.0f), 1u };
        auto range0 = TextureViewRange(0, 0, 0, 2);
        auto range1 = TextureViewRange(0, 2, 0, 2);

        GraphicsAPI::SetTexture(hash->pk_ScreenGI_SHY_Read, m_screenSpaceSHY.get(), range1);
        GraphicsAPI::SetTexture(hash->pk_ScreenGI_CoCg_Read, m_screenSpaceCoCg.get(), range1);
        GraphicsAPI::SetImage(hash->pk_ScreenGI_SHY_Write, m_screenSpaceSHY.get(), range0);
        GraphicsAPI::SetImage(hash->pk_ScreenGI_CoCg_Write, m_screenSpaceCoCg.get(), range0);
        cmd->Dispatch(m_computeReprojectMask, 0, groupSize);

        GraphicsAPI::SetTexture(hash->pk_ScreenGI_SHY_Read, m_screenSpaceSHY.get(), range0);
        GraphicsAPI::SetTexture(hash->pk_ScreenGI_CoCg_Read, m_screenSpaceCoCg.get(), range0);
        GraphicsAPI::SetImage(hash->pk_ScreenGI_SHY_Write, m_screenSpaceSHY.get(), range1);
        GraphicsAPI::SetImage(hash->pk_ScreenGI_CoCg_Write, m_screenSpaceCoCg.get(), range1);
        cmd->Dispatch(m_computeDenoise, 0, groupSize);

        cmd->EndDebugScope();

        cmd->BeginDebugScope("SceneGI.Voxelize", PK_COLOR_GREEN);

        auto volres = m_voxels->GetResolution();

        uint4 viewports[3] =
        {
            {0u, 0u, volres.x, volres.z },
            {0u, 0u, volres.x, volres.y },
            {0u, 0u, volres.y, volres.z },
        };

        cmd->SetRenderTarget({ viewports[m_rasterAxis].z, viewports[m_rasterAxis].w, 1 });
        cmd->SetViewPort(viewports[m_rasterAxis]);
        cmd->SetScissor(viewports[m_rasterAxis]);

        batcher->Render(cmd, batchGroup, &m_voxelizeAttribs, hash->PK_META_PASS_GIVOXELIZE);

        GraphicsAPI::SetTexture(hash->_SourceTex, m_voxels.get());

        for (auto i = 1u; i < m_voxels->GetLevels(); ++i)
        {
            GraphicsAPI::SetImage(hash->_DestinationTex, m_voxels.get(), i, 0);
            cmd->Dispatch(m_computeMipmap, 0, { (volres.x >> i) / 4u, (volres.y >> i) / 4u, (volres.z >> i) / 4u });
        }

        cmd->EndDebugScope();
    }

    void PassSceneGI::RenderGI(CommandBuffer* cmd)
    {
        cmd->BeginDebugScope("SceneGI.Gather", PK_COLOR_GREEN);

        auto hash = HashCache::Get();
        auto resolution = m_screenSpaceSHY->GetResolution();
        uint3 groupSize = { (uint)ceil(resolution.x / 16.0f), (uint)ceil(resolution.y / 16.0f), 1u };
        auto range0 = TextureViewRange(0, 0, 0, 2);
        auto range1 = TextureViewRange(0, 2, 0, 2);

        GraphicsAPI::SetTexture(hash->pk_ScreenGI_SHY_Read, m_screenSpaceSHY.get(), range1);
        GraphicsAPI::SetTexture(hash->pk_ScreenGI_CoCg_Read, m_screenSpaceCoCg.get(), range1);
        GraphicsAPI::SetImage(hash->pk_ScreenGI_SHY_Write, m_screenSpaceSHY.get(), range0);
        GraphicsAPI::SetImage(hash->pk_ScreenGI_CoCg_Write, m_screenSpaceCoCg.get(), range0);
        cmd->Dispatch(m_computeBakeGI, 0, groupSize);

        GraphicsAPI::SetTexture(hash->pk_ScreenGI_SHY_Read, m_screenSpaceSHY.get(), range0);
        GraphicsAPI::SetTexture(hash->pk_ScreenGI_CoCg_Read, m_screenSpaceCoCg.get(), range0);
        GraphicsAPI::SetImage(hash->pk_ScreenGI_SHY_Write, m_screenSpaceSHY.get(), range1);
        GraphicsAPI::SetImage(hash->pk_ScreenGI_CoCg_Write, m_screenSpaceCoCg.get(), range1);
        cmd->Dispatch(m_computeDenoise, 0, groupSize);

        GraphicsAPI::SetTexture(hash->pk_ScreenGI_SHY_Read, m_screenSpaceSHY.get(), range1);
        GraphicsAPI::SetTexture(hash->pk_ScreenGI_CoCg_Read, m_screenSpaceCoCg.get(), range1);
        cmd->EndDebugScope();
    }

    void PassSceneGI::CacheForwardOutput(Objects::CommandBuffer* cmd, Objects::Texture* texture)
    {
        cmd->Blit(texture, m_previousForwardOutput.get(), {}, {}, FilterMode::Point);
    }
}