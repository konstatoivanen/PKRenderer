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
        m_computeMask = assetDatabase->Find<Shader>("CS_SceneGI_Mask");

        TextureDescriptor descr{};
        descr.samplerType = SamplerType::Sampler3D;
        descr.format = TextureFormat::RGBA16;
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
        m_voxels = Texture::Create(descr, "GI Voxel Volume");

        descr.format = TextureFormat::R8UI;
        descr.sampler.borderColor = BorderColor::IntClear;
        descr.levels = 1u;
        descr.sampler.mipMax = 0.0f;
        m_voxelMask = Texture::Create(descr, "GI Voxel Volume Mask");

        descr.samplerType = SamplerType::Sampler2D;
        descr.format = TextureFormat::R8UI;
        descr.layers = 1u;
        descr.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        m_mask = Texture::Create(descr, "GI Screen Space Mask Texture");

        descr.samplerType = SamplerType::Sampler2DArray;
        descr.format = TextureFormat::RGBA16F;
        descr.sampler.filterMin = FilterMode::Bilinear;
        descr.sampler.filterMag = FilterMode::Bilinear;
        descr.sampler.wrap[0] = WrapMode::Clamp;
        descr.sampler.wrap[1] = WrapMode::Clamp;
        descr.sampler.wrap[2] = WrapMode::Clamp;
        descr.levels = 1u;
        descr.layers = 4u;
        descr.usage = TextureUsage::RTColorSample | TextureUsage::Storage;
        m_screenSpaceGI = Texture::Create(descr, "GI Sceen Space Texture");


        auto cmd = GraphicsAPI::GetCommandBuffer();
        auto hash = HashCache::Get();
        cmd->SetImage(hash->pk_SceneGI_VolumeMaskWrite, m_voxelMask.get());
        cmd->SetImage(hash->pk_SceneGI_VolumeWrite, m_voxels.get());
        cmd->SetTexture(hash->pk_SceneGI_VolumeRead, m_voxels.get());
        cmd->SetImage(hash->pk_ScreenGI_Mask, m_mask.get());

        m_voxelizeAttribs.depthStencil.depthCompareOp = Comparison::Off;
        m_voxelizeAttribs.depthStencil.depthWriteEnable = false;
        m_voxelizeAttribs.rasterization.cullMode = CullMode::Off;

        m_parameters = CreateRef<ConstantBuffer>(BufferLayout(
        {
            { ElementType::Float4, hash->pk_SceneGI_ST },
            { ElementType::Uint4, hash->pk_SceneGI_Swizzle },
            { ElementType::Int4, hash->pk_SceneGI_Checkerboard_Offset },
            { ElementType::Float, hash->pk_SceneGI_VoxelSize },
            { ElementType::Float, hash->pk_SceneGI_ConeAngle },
            { ElementType::Float, hash->pk_SceneGI_DiffuseGain },
            { ElementType::Float, hash->pk_SceneGI_SpecularGain },
            { ElementType::Float, hash->pk_SceneGI_Fade }
        }), "Scene GI Parameters");

        m_parameters->Set<float4>(hash->pk_SceneGI_ST, float4(-76.8f, -6.0f, -76.8f, 1.0f / 0.6f));
        m_parameters->Set<float>(hash->pk_SceneGI_VoxelSize, 0.6f);
        m_parameters->Set<float>(hash->pk_SceneGI_ConeAngle, 5.08320368996f);
        m_parameters->Set<float>(hash->pk_SceneGI_DiffuseGain, 2.0f);
        m_parameters->Set<float>(hash->pk_SceneGI_SpecularGain, 1.0f);
        m_parameters->Set<float>(hash->pk_SceneGI_Fade, 0.95f);
        cmd->SetBuffer(hash->pk_SceneGI_Params, m_parameters->GetBuffer());
    }

    void PassSceneGI::PreRender(CommandBuffer* cmd, const uint3& resolution)
    {
        auto hash = HashCache::Get();

        m_screenSpaceGI->Validate(resolution);

        if (m_mask->Validate(resolution))
        {
            cmd->SetImage(hash->pk_ScreenGI_Mask, m_mask.get());
        }

        uint4 swizzles[3] =
        {
             { 0u, 1u, 2u, 0u },
             { 1u, 2u, 0u, 0u },
             { 0u, 2u, 1u, 0u }
        };

        m_rasterAxis = (m_rasterAxis + 1) % 3;
        m_checkerboardIndex = (m_checkerboardIndex + 1) % 4;

        m_parameters->Set<uint4>(hash->pk_SceneGI_Swizzle, swizzles[m_rasterAxis]);
        m_parameters->Set<int4>(hash->pk_SceneGI_Checkerboard_Offset, { m_checkerboardIndex / 2, m_checkerboardIndex % 2, 0, 0 });
        m_parameters->FlushBuffer();
    }

    void PassSceneGI::RenderVoxels(CommandBuffer* cmd, Batcher* batcher, uint32_t batchGroup)
    {
        cmd->BeginDebugScope("GI Voxelize", PK_COLOR_GREEN);

        auto hash = HashCache::Get();

        auto volres = m_voxels->GetResolution();

        uint4 viewports[3] =
        {
            {0u, 0u, volres.x, volres.y },
            {0u, 0u, volres.y, volres.z },
            {0u, 0u, volres.x, volres.z },
        };

        cmd->SetRenderTarget({ viewports[m_rasterAxis].z, viewports[m_rasterAxis].w, 1 });
        cmd->SetViewPort(viewports[m_rasterAxis]);
        cmd->SetScissor(viewports[m_rasterAxis]);

        batcher->Render(cmd, batchGroup, &m_voxelizeAttribs, hash->PK_META_PASS_GIVOXELIZE);

        // Clear transparencies every axis cycle
        if (m_rasterAxis == 0)
        {
            cmd->SetImage(hash->_DestinationTex, m_voxels.get(), 0, 0);
            cmd->Dispatch(m_computeClear, { volres.x / 8u, volres.y / 8u, volres.z / 8u });
            cmd->Barrier(m_voxels.get(), 0, 0, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::FragmentReadWrite);
        }

        cmd->SetTexture(hash->_SourceTex, m_voxels.get());

        for (auto i = 1u; i < m_voxels->GetLevels(); ++i)
        {
            cmd->SetImage(hash->_DestinationTex, m_voxels.get(), i, 0);
            cmd->Dispatch(m_computeMipmap, 0, { (volres.x >> i) / 4u, (volres.y >> i) / 4u, (volres.z >> i) / 4u });
            cmd->Barrier(m_voxels.get(), i, 0, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::ComputeRead);
        }

        cmd->EndDebugScope();
    }
    
    void PassSceneGI::RenderGI(CommandBuffer* cmd)
    {
        cmd->BeginDebugScope("GI Gather", PK_COLOR_GREEN);

        auto hash = HashCache::Get();

        auto resolution = m_screenSpaceGI->GetResolution();
        uint3 groupSize = { (uint)ceil(resolution.x / 16.0f), (uint)ceil(resolution.y / 16.0f), 1u };

        cmd->Blit(m_screenSpaceGI.get(), m_screenSpaceGI.get(), { 0, 0, 0, 2 }, { 0, 2, 0, 2 }, FilterMode::Point);

        {
            cmd->SetTexture(hash->pk_ScreenGI_Read, m_screenSpaceGI.get(), { 0, 2, 0, 2 });
            cmd->SetImage(hash->pk_ScreenGI_Write, m_screenSpaceGI.get(), { 0, 0, 0, 2 });
            cmd->SetImage(hash->_DestinationTex, m_mask.get());
            
            cmd->Dispatch(m_computeMask, 0, groupSize);
            
            cmd->Barrier(m_mask.get(), 0, 0, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::ComputeRead);
            cmd->Barrier(m_screenSpaceGI.get(), { 0, 0, 0, 2 }, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::ComputeWrite);
        }

        {
            cmd->Dispatch(m_computeBakeGI, 0, groupSize);
            cmd->Barrier(m_screenSpaceGI.get(), { 0, 0, 0, 2 }, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::FragmentTexture);
            cmd->SetTexture(hash->pk_ScreenGI_Read, m_screenSpaceGI.get(), { 0, 0, 0, 2 });
        }

        cmd->EndDebugScope();
    }
}