#include "PrecompiledHeader.h"
#include "Core/Services/Log.h"
#include "RenderPipeline.h"
#include "Rendering/MeshUtility.h"
#include "Rendering/HashCache.h"

namespace PK::Rendering
{
    using namespace Utilities;
    using namespace Core;
    using namespace Core::Services;
    using namespace Math;
    using namespace ECS;
    using namespace Objects;
    using namespace Structs;

    struct ModelMatrices
    {
        float4x4 pk_MATRIX_M;
        float4x4 pk_MATRIX_I_M;
    };

    RenderPipeline::RenderPipeline(AssetDatabase* assetDatabase, EntityDatabase* entityDb, Sequencer* sequencer, const ApplicationConfig* config) :
        m_passPostEffects(assetDatabase, config),
        m_passGeometry(entityDb, sequencer, &m_batcher),
        m_passLights(assetDatabase, entityDb, sequencer, &m_batcher, config),
        m_passSceneGI(assetDatabase, config),
        m_passVolumeFog(assetDatabase, config),
        m_batcher(),
        m_visibilityList(1024)
    {
        m_OEMBackgroundShader = assetDatabase->Find<Shader>("SH_VS_IBLBackground");

        RenderTextureDescriptor descriptor{};
        descriptor.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        descriptor.colorFormats[0] = TextureFormat::RGBA16F;
        descriptor.colorFormats[1] = TextureFormat::RGBA16F;
        descriptor.depthFormat = TextureFormat::Depth32F;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        descriptor.sampler.filter = FilterMode::Bilinear;
        m_RenderTarget = CreateRef<RenderTexture>(descriptor);

        auto hash = HashCache::Get();

        m_constantsPerFrame = CreateRef<ConstantBuffer>(BufferLayout(
        { 
            { ElementType::Float4, hash->pk_Time },
            { ElementType::Float4, hash->pk_SinTime },
            { ElementType::Float4, hash->pk_CosTime },
            { ElementType::Float4, hash->pk_DeltaTime },
            { ElementType::Float4, hash->pk_CursorParams },
            { ElementType::Float4, hash->pk_WorldSpaceCameraPos },
            { ElementType::Float4, hash->pk_ProjectionParams },
            { ElementType::Float4, hash->pk_ExpProjectionParams },
            { ElementType::Float4, hash->pk_ScreenParams },
            { ElementType::Float4, hash->pk_ShadowCascadeZSplits },
            { ElementType::Float4x4, hash->pk_MATRIX_V },
            { ElementType::Float4x4, hash->pk_MATRIX_I_V },
            { ElementType::Float4x4, hash->pk_MATRIX_P },
            { ElementType::Float4x4, hash->pk_MATRIX_I_P },
            { ElementType::Float4x4, hash->pk_MATRIX_VP },
            { ElementType::Float4x4, hash->pk_MATRIX_I_VP },
            { ElementType::Float4x4, hash->pk_MATRIX_L_VP },
            { ElementType::Float4x4, hash->pk_MATRIX_LD_P },
            { ElementType::Float, hash->pk_SceneOEM_Exposure }
        }));

        m_constantsPerFrame->Set<float>(hash->pk_SceneOEM_Exposure, config->BackgroundExposure);

        auto bluenoise = assetDatabase->Load<Texture>("res/textures/default/T_Bluenoise256.ktx2");
        auto lightCookies = assetDatabase->Load<Texture>("res/textures/default/T_LightCookies.ktx2");
        auto sceneOEM = assetDatabase->Load<Texture>(config->FileBackgroundTexture.value.c_str());
        
        auto sampler = lightCookies->GetSamplerDescriptor();
        sampler.wrap[0] = WrapMode::Clamp;
        sampler.wrap[1] = WrapMode::Clamp;
        sampler.wrap[2] = WrapMode::Clamp;
        lightCookies->SetSampler(sampler);

        sampler = sceneOEM->GetSamplerDescriptor();
        sampler.wrap[0] = WrapMode::Mirror;
        sampler.wrap[1] = WrapMode::Mirror;
        sampler.wrap[2] = WrapMode::Mirror;
        sceneOEM->SetSampler(sampler);

        sampler = bluenoise->GetSamplerDescriptor();
        sampler.anisotropy = 0.0f;
        sampler.mipMin = 0.0f;
        sampler.mipMax = 0.0f;
        bluenoise->SetSampler(sampler);

        auto cmd = GraphicsAPI::GetCommandBuffer();

        cmd->SetTexture(hash->pk_Bluenoise256, bluenoise);
        cmd->SetTexture(hash->pk_LightCookies, lightCookies);
        cmd->SetTexture(hash->pk_SceneOEM_HDR, sceneOEM);
        cmd->SetBuffer(hash->pk_PerFrameConstants, *m_constantsPerFrame.get());

        PK_LOG_VERBOSE("Render Pipeline Initialized!");
    }
    
    RenderPipeline::~RenderPipeline()
    {
        GraphicsAPI::GetActiveDriver()->WaitForIdle();
        m_constantsPerFrame = nullptr;
        m_RenderTarget = nullptr;
    }

    void RenderPipeline::Step(PK::ECS::Tokens::ViewProjectionUpdateToken* token)
    {
        auto hash = HashCache::Get();

        auto cameraMatrix = glm::inverse(token->view);

        auto n = Functions::GetZNearFromProj(token->projection);
        auto f = Functions::GetZFarFromProj(token->projection);
        auto vp = token->projection * token->view;
        auto pvp = vp;

        m_viewProjectionMatrix = vp;
        m_znear = n;
        m_zfar = f;

        m_constantsPerFrame->TryGet(hash->pk_MATRIX_VP, pvp);
        m_constantsPerFrame->Set<float4>(hash->pk_ProjectionParams, { n, f, f - n, 1.0f / f });
        m_constantsPerFrame->Set<float4>(hash->pk_ExpProjectionParams, { 1.0f / glm::log2(f / n), -log2(n) / log2(f / n), f / n, 1.0f / n });
        m_constantsPerFrame->Set<float4>(hash->pk_WorldSpaceCameraPos, cameraMatrix[3]);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_V, token->view);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_I_V, cameraMatrix);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_P, token->projection);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_I_P, glm::inverse(token->projection));
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_VP, vp);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_I_VP, glm::inverse(vp));
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_L_VP, pvp);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_LD_P, pvp * cameraMatrix);
    }

    void RenderPipeline::Step(PK::ECS::Tokens::TimeToken* token)
    {
        auto* hash = HashCache::Get();
        m_constantsPerFrame->Set<float4>(hash->pk_Time, { (float)token->time / 20.0f, (float)token->time, (float)token->time * 2.0f, (float)token->time * 3.0f });
        m_constantsPerFrame->Set<float4>(hash->pk_SinTime, { (float)sin(token->time / 8.0f), (float)sin(token->time / 4.0f), (float)sin(token->time / 2.0f), (float)sin(token->time) });
        m_constantsPerFrame->Set<float4>(hash->pk_CosTime, { (float)cos(token->time / 8.0f), (float)cos(token->time / 4.0f), (float)cos(token->time / 2.0f), (float)cos(token->time) });
        m_constantsPerFrame->Set<float4>(hash->pk_DeltaTime, { (float)token->deltaTime, 1.0f / (float)token->deltaTime, (float)token->smoothDeltaTime, 1.0f / (float)token->smoothDeltaTime });
        token->logFrameRate = true;
    }
    
    void RenderPipeline::Step(Window* window, int condition)
    {
        auto hash = HashCache::Get();
        auto* cmd = GraphicsAPI::GetCommandBuffer();
        auto resolution = window->GetResolution();

        m_RenderTarget->Validate(resolution);
        cmd->SetTexture(hash->pk_ScreenDepth, m_RenderTarget->GetDepth());
        cmd->SetTexture(hash->pk_ScreenNormals, m_RenderTarget->GetColor(1));

        auto cascadeZSplits = m_passLights.GetCascadeZSplits(m_znear, m_zfar);
        m_constantsPerFrame->Set<float4>(hash->pk_ShadowCascadeZSplits, reinterpret_cast<float4*>(cascadeZSplits.planes));
        m_constantsPerFrame->Set<float4>(hash->pk_ScreenParams, { (float)resolution.x, (float)resolution.y, 1.0f / (float)resolution.x, 1.0f / (float)resolution.y });
        m_constantsPerFrame->FlushBuffer();

        m_batcher.BeginCollectDrawCalls();
        m_passGeometry.Cull(this, &m_visibilityList, m_viewProjectionMatrix, m_zfar - m_znear);
        m_passLights.Cull(this, &m_visibilityList, m_viewProjectionMatrix, m_znear, m_zfar);
        m_batcher.EndCollectDrawCalls();

        m_passSceneGI.PreRender(cmd, resolution);
        m_passLights.Render(cmd);
        m_passSceneGI.RenderVoxels(cmd, &m_batcher, m_passGeometry.GetPassGroup());

        cmd->SetRenderTarget(m_RenderTarget.get(), { 1 }, true, true);
        cmd->ClearColor({ 0, 0, -1.0f, 1.0f }, 0);
        cmd->ClearDepth(1.0f, 0u);

        m_passGeometry.RenderGBuffer(cmd);
        m_passSceneGI.RenderGI(cmd);

        cmd->SetRenderTarget(m_RenderTarget.get(), { 0 }, true, true);
        cmd->ClearColor(PK_COLOR_CLEAR, 0);

        m_passGeometry.RenderForward(cmd);
        cmd->Blit(m_OEMBackgroundShader);

        m_passVolumeFog.Render(cmd, m_RenderTarget.get(), resolution);
        m_passPostEffects.Render(cmd, m_RenderTarget.get(), MemoryAccessFlags::FragmentAttachmentColor);

        cmd->Blit(m_RenderTarget->GetColor(0), window, FilterMode::Bilinear);
    }

    void RenderPipeline::Step(AssetImportToken<ApplicationConfig>* token)
    {
        auto tex = token->assetDatabase->Load<Texture>(token->asset->FileBackgroundTexture.value.c_str());
        auto sampler = tex->GetSamplerDescriptor();
        sampler.wrap[0] = WrapMode::Mirror;
        sampler.wrap[1] = WrapMode::Mirror;
        sampler.wrap[2] = WrapMode::Mirror;
        tex->SetSampler(sampler);

        GraphicsAPI::GetCommandBuffer()->SetTexture(HashCache::Get()->pk_SceneOEM_HDR, tex);
        m_constantsPerFrame->Set<float>(HashCache::Get()->pk_SceneOEM_Exposure, token->asset->BackgroundExposure);
        m_passPostEffects.OnUpdateParameters(token->asset);
        m_passVolumeFog.OnUpdateParameters(token->asset);
    }
}