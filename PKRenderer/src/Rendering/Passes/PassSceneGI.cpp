#include "PrecompiledHeader.h"
#include "PassSceneGI.h"
#include "Rendering/HashCache.h"

namespace PK::Rendering::Passes
{
    PassSceneGI::PassSceneGI(AssetDatabase* assetDatabase, const ApplicationConfig* config)
    {
        m_computeFade = assetDatabase->Find<Shader>("CS_SceneGI_Fade");
        m_computeMipmap = assetDatabase->Find<Shader>("CS_SceneGI_Mipmap");
        m_computeBakeGI = assetDatabase->Find<Shader>("CS_SceneGI_Bake");

        uint3 resolution = { 256u, 128u, 256u };

        TextureDescriptor descr{};
        descr.samplerType = SamplerType::Sampler3D;
        descr.format = TextureFormat::RGBA16;
        descr.sampler.filter = FilterMode::Trilinear;
        descr.sampler.wrap[0] = WrapMode::Border;
        descr.sampler.wrap[1] = WrapMode::Border;
        descr.sampler.wrap[2] = WrapMode::Border;
        descr.sampler.borderColor = BorderColor::FloatClear;
        descr.sampler.mipMax = 6.0f;
        descr.resolution = resolution;
        descr.levels = 7u;
        descr.usage = TextureUsage::Sample | TextureUsage::Storage;
        m_voxels = Texture::Create(descr);

        descr.samplerType = SamplerType::Sampler2DArray;
        descr.format = TextureFormat::RGBA16F;
        descr.sampler.filter = FilterMode::Bilinear;
        descr.sampler.wrap[0] = WrapMode::Clamp;
        descr.sampler.wrap[1] = WrapMode::Clamp;
        descr.sampler.wrap[2] = WrapMode::Clamp;
        descr.levels = 1u;
        descr.layers = 2u;
        descr.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        m_screenSpaceGI = Texture::Create(descr);

        auto cmd = GraphicsAPI::GetCommandBuffer();
        auto hash = HashCache::Get();
        cmd->SetImage(hash->pk_SceneGI_VolumeWrite, m_voxels.get());
        cmd->SetImage(hash->pk_ScreenGI_Write, m_screenSpaceGI.get());
        cmd->SetTexture(hash->pk_SceneGI_VolumeRead, m_voxels.get());
        cmd->SetTexture(hash->pk_ScreenGI_Read, m_screenSpaceGI.get());

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
        }));

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

        if (m_screenSpaceGI->Validate(resolution))
        {
            cmd->SetImage(hash->pk_ScreenGI_Write, m_screenSpaceGI.get());
            cmd->SetTexture(hash->pk_ScreenGI_Read, m_screenSpaceGI.get());
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
        auto hash = HashCache::Get();

        auto volres = m_voxels->GetResolution();

        cmd->SetImage(hash->_DestinationTex, m_voxels.get(), 0, 0);
        cmd->Dispatch(m_computeFade, { volres.x / 8u, volres.y / 8u, volres.z / 8u });
        cmd->Barrier(m_voxels.get(), 0, 0, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::FragmentReadWrite);

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

        cmd->SetTexture(hash->_SourceTex, m_voxels.get());

        for (auto i = 1u; i < m_voxels->GetLevels(); ++i)
        {
            cmd->SetImage(hash->_DestinationTex, m_voxels.get(), i, 0);
            cmd->Dispatch(m_computeMipmap, 0, { (volres.x >> i) / 4u, (volres.y >> i) / 4u, (volres.z >> i) / 4u });
            cmd->Barrier(m_voxels.get(), i, 0, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::ComputeRead);
        }
    }
    
    void PassSceneGI::RenderGI(CommandBuffer* cmd)
    {
        auto resolution = m_screenSpaceGI->GetResolution();
        cmd->Dispatch(m_computeBakeGI, 0, { (uint)ceil(resolution.x / 32.0f), (uint)ceil(resolution.y / 32.0f), 1u });
        cmd->Barrier(m_screenSpaceGI.get(), MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::FragmentTexture);
    }
}