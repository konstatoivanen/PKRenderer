#include "PrecompiledHeader.h"
#include "Core/Services/Log.h"
#include "RenderPipeline.h"
#include "Rendering/MeshUtility.h"
#include "Rendering/HashCache.h"

namespace PK::Rendering
{
    using namespace PK::Rendering::Objects;

    struct ModelMatrices
    {
        float4x4 pk_MATRIX_M;
        float4x4 pk_MATRIX_I_M;
    };

    RenderPipeline::RenderPipeline(AssetDatabase* assetDatabase, EntityDatabase* entityDb, Sequencer* sequencer, const ApplicationConfig* config) :
        m_passPostEffects(assetDatabase, config),
        m_passGeometry(entityDb, sequencer, &m_batcher),
        m_passLights(entityDb, sequencer, &m_batcher, config->CascadeLinearity),
        m_batcher()
    {
        m_OEMBackgroundShader = assetDatabase->Find<Shader>("SH_VS_IBLBackground");

        RenderTextureDescriptor descriptor{};
        descriptor.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        descriptor.colorFormats[0] = TextureFormat::RGBA16F;
        descriptor.depthFormat = TextureFormat::Depth32F;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        descriptor.sampler.filter = FilterMode::Bilinear;
        m_HDRRenderTarget = CreateRef<RenderTexture>(descriptor);

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
            { ElementType::Float, hash->pk_SceneOEM_Exposure }
        }));

        m_constantsPerFrame->Set<float>(hash->pk_SceneOEM_Exposure, config->BackgroundExposure);

        auto cmd = GraphicsAPI::GetCommandBuffer();
        cmd->SetTexture(hash->pk_Bluenoise256, assetDatabase->Load<Texture>("res/textures/T_Bluenoise256.ktx2"));
        cmd->SetTexture(hash->pk_LightCookies, assetDatabase->Load<Texture>("res/textures/T_LightCookies.ktx2"));
        cmd->SetBuffer(hash->pk_PerFrameConstants, *m_constantsPerFrame.get());
        cmd->SetTexture(hash->pk_SceneOEM_HDR, assetDatabase->Load<Texture>(config->FileBackgroundTexture.value.c_str()));

        m_visibilityList.results.resize(1024);

        PK_LOG_VERBOSE("Render Pipeline Initialized!");
    }
    
    RenderPipeline::~RenderPipeline()
    {
        GraphicsAPI::GetActiveDriver()->WaitForIdle();
        m_constantsPerFrame = nullptr;
        m_HDRRenderTarget = nullptr;
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

        m_batcher.Reset();

        m_passGeometry.Cull(this, &m_visibilityList, m_viewProjectionMatrix, m_zfar - m_znear);
        m_passLights.Cull(this, &m_visibilityList, m_viewProjectionMatrix, m_znear, m_zfar);

        m_batcher.BuildDrawCalls();

        m_passLights.RenderShadowmaps(cmd);

        auto resolution = window->GetResolution();
        m_HDRRenderTarget->Validate(resolution);
        m_constantsPerFrame->Set<float4>(hash->pk_ScreenParams, { (float)resolution.x, (float)resolution.y, 1.0f / (float)resolution.x, 1.0f / (float)resolution.y });
        m_constantsPerFrame->FlushBuffer();

        cmd->SetTexture(hash->pk_ScreenDepth, m_HDRRenderTarget->GetDepth());

        m_passLights.RenderTiles(cmd);

        cmd->SetRenderTarget(m_HDRRenderTarget.get());
        cmd->ClearColor(PK_COLOR_RED, 0);
        cmd->ClearDepth(1.0f, 0u);

        cmd->Blit(m_OEMBackgroundShader);

        m_passGeometry.Render(cmd);
        m_passPostEffects.Execute(m_HDRRenderTarget.get(), MemoryAccessFlags::FragmentAttachmentColor);

        cmd->Blit(m_HDRRenderTarget->GetColor(0), window, 0, 0, FilterMode::Bilinear);
    }

    void RenderPipeline::Step(AssetImportToken<ApplicationConfig>* token)
    {
        auto tex = token->assetDatabase->Load<Texture>(token->asset->FileBackgroundTexture.value.c_str());
        GraphicsAPI::GetCommandBuffer()->SetTexture(HashCache::Get()->pk_SceneOEM_HDR, tex);
        m_constantsPerFrame->Set<float>(HashCache::Get()->pk_SceneOEM_Exposure, token->asset->BackgroundExposure);
        m_passPostEffects.OnUpdateParameters(token->asset);
    }
}