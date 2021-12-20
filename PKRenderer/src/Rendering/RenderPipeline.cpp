#include "PrecompiledHeader.h"
#include "RenderPipeline.h"
#include "Rendering/MeshUtility.h"
#include "Rendering/HashCache.h"
#include "Utilities/Log.h"

namespace PK::Rendering
{
    using namespace PK::Rendering::Objects;

    struct ModelMatrices
    {
        float4x4 pk_MATRIX_M;
        float4x4 pk_MATRIX_I_M;
    };

    RenderPipeline::RenderPipeline(AssetDatabase* assetDatabase, const ApplicationConfig* config) :
        m_passPostEffects(assetDatabase, config)
    {
        m_OEMBackgroundShader = assetDatabase->Find<Shader>("SH_VS_IBLBackground");
        m_OEMTexture = assetDatabase->Load<Texture>(config->FileBackgroundTexture.value.c_str());

        RenderTextureDescriptor descriptor{};
        descriptor.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        descriptor.colorFormats[0] = TextureFormat::RGBA16F;
        descriptor.depthFormat = TextureFormat::Depth32F;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        descriptor.sampler.filter = FilterMode::Bilinear;
        m_HDRRenderTarget = CreateRef<RenderTexture>(descriptor);

        auto hashCache = HashCache::Get();

        m_constantsPerFrame = CreateRef<ConstantBuffer>(BufferLayout(
        { 
            { ElementType::Float4, hashCache->pk_Time },
            { ElementType::Float4, hashCache->pk_SinTime },
            { ElementType::Float4, hashCache->pk_CosTime },
            { ElementType::Float4, hashCache->pk_DeltaTime },
            { ElementType::Float4, hashCache->pk_CursorParams },
            { ElementType::Float4, hashCache->pk_WorldSpaceCameraPos },
            { ElementType::Float4, hashCache->pk_ProjectionParams },
            { ElementType::Float4, hashCache->pk_ExpProjectionParams },
            { ElementType::Float4, hashCache->pk_ScreenParams },
            { ElementType::Float4, hashCache->pk_ShadowCascadeZSplits },
            { ElementType::Float4x4, hashCache->pk_MATRIX_V },
            { ElementType::Float4x4, hashCache->pk_MATRIX_I_V },
            { ElementType::Float4x4, hashCache->pk_MATRIX_P },
            { ElementType::Float4x4, hashCache->pk_MATRIX_I_P },
            { ElementType::Float4x4, hashCache->pk_MATRIX_VP },
            { ElementType::Float4x4, hashCache->pk_MATRIX_I_VP },
            { ElementType::Float4x4, hashCache->pk_MATRIX_L_VP },
            { ElementType::Float, hashCache->pk_SceneOEM_Exposure }
        }));

        m_modelMatrices = Buffer::CreateConstant(
        {
            { ElementType::Float4x4, hashCache->pk_MATRIX_M },
            { ElementType::Float4x4, hashCache->pk_MATRIX_I_M },
        });

        m_constantsPerFrame->Set<float>(hashCache->pk_SceneOEM_Exposure, config->BackgroundExposure);

        m_shader = assetDatabase->Find<Shader>("SH_WS_Debug");
        m_mesh = assetDatabase->Load<Mesh>("res/models/MDL_Cloth.pkmesh");
        m_testTexture = assetDatabase->Load<Texture>("res/textures/T_DebugTexture.ktx2");

        PK_LOG_VERBOSE("Render Pipeline Initialized!");
    }
    
    RenderPipeline::~RenderPipeline()
    {
        GraphicsAPI::GetActiveDriver()->WaitForIdle();
        m_mesh = nullptr;
        m_constantsPerFrame = nullptr;
        m_HDRRenderTarget = nullptr;
        m_shader = nullptr;
    }

    void RenderPipeline::Step(PK::ECS::Tokens::ViewProjectionUpdateToken* token)
    {
        auto hashCache = HashCache::Get();

        auto cameraMatrix = glm::inverse(token->view);

        auto n = Functions::GetZNearFromProj(token->projection);
        auto f = Functions::GetZFarFromProj(token->projection);
        auto vp = token->projection * token->view;
        auto pvp = vp;

        m_constantsPerFrame->TryGet(hashCache->pk_MATRIX_VP, pvp);
        m_constantsPerFrame->Set<float4>(hashCache->pk_ProjectionParams, { n, f, f - n, 1.0f / f });
        m_constantsPerFrame->Set<float4>(hashCache->pk_ExpProjectionParams, { 1.0f / glm::log2(f / n), -log2(n) / log2(f / n), f / n, 1.0f / n });
        m_constantsPerFrame->Set<float4>(hashCache->pk_WorldSpaceCameraPos, cameraMatrix[3]);
        m_constantsPerFrame->Set<float4x4>(hashCache->pk_MATRIX_V, token->view);
        m_constantsPerFrame->Set<float4x4>(hashCache->pk_MATRIX_I_V, cameraMatrix);
        m_constantsPerFrame->Set<float4x4>(hashCache->pk_MATRIX_P, token->projection);
        m_constantsPerFrame->Set<float4x4>(hashCache->pk_MATRIX_I_P, glm::inverse(token->projection));
        m_constantsPerFrame->Set<float4x4>(hashCache->pk_MATRIX_VP, vp);
        m_constantsPerFrame->Set<float4x4>(hashCache->pk_MATRIX_I_VP, glm::inverse(vp));
        m_constantsPerFrame->Set<float4x4>(hashCache->pk_MATRIX_L_VP, pvp);
    }

    void RenderPipeline::Step(PK::ECS::Tokens::TimeToken* token)
    {
        auto* hashCache = HashCache::Get();
        m_constantsPerFrame->Set<float4>(hashCache->pk_Time, { (float)token->time / 20.0f, (float)token->time, (float)token->time * 2.0f, (float)token->time * 3.0f });
        m_constantsPerFrame->Set<float4>(hashCache->pk_SinTime, { (float)sin(token->time / 8.0f), (float)sin(token->time / 4.0f), (float)sin(token->time / 2.0f), (float)sin(token->time) });
        m_constantsPerFrame->Set<float4>(hashCache->pk_CosTime, { (float)cos(token->time / 8.0f), (float)cos(token->time / 4.0f), (float)cos(token->time / 2.0f), (float)cos(token->time) });
        m_constantsPerFrame->Set<float4>(hashCache->pk_DeltaTime, { (float)token->deltaTime, 1.0f / (float)token->deltaTime, (float)token->smoothDeltaTime, 1.0f / (float)token->smoothDeltaTime });
        token->logFrameRate = true;
    }
    
    void RenderPipeline::Step(Window* window, int condition)
    {
        auto hash = HashCache::Get();
        auto* cmd = GraphicsAPI::GetCommandBuffer();

        auto resolution = window->GetResolution();
        m_HDRRenderTarget->Validate(resolution);

        m_constantsPerFrame->Set<float4>(hash->pk_ScreenParams, { (float)resolution.x, (float)resolution.y, 1.0f / (float)resolution.x, 1.0f / (float)resolution.y });
        m_constantsPerFrame->FlushBuffer();

        auto modelView = m_modelMatrices->BeginMap<ModelMatrices>();
        modelView[0].pk_MATRIX_M = PK_FLOAT4X4_IDENTITY; 
        modelView[0].pk_MATRIX_I_M = glm::inverse(modelView[0].pk_MATRIX_M);
        m_modelMatrices->EndMap();

        cmd->SetTexture(hash->pk_ScreenDepth, m_HDRRenderTarget->GetDepth());
        cmd->SetRenderTarget(m_HDRRenderTarget.get());
        cmd->ClearColor(PK_COLOR_RED, 0);
        cmd->ClearDepth(1.0f, 0u);

        cmd->SetBuffer(hash->pk_PerFrameConstants, *m_constantsPerFrame.get());
        cmd->SetBuffer(hash->pk_ModelMatrices, m_modelMatrices.get());
        cmd->SetTexture(hash->pk_SceneOEM_HDR, m_OEMTexture);
        cmd->SetTexture("tex1", m_testTexture);
        cmd->SetConstant<float4>("offset", { 0, 0, 0, 0 });

        cmd->Blit(m_OEMBackgroundShader);
        cmd->DrawMesh(m_mesh, 0, m_shader);

        m_passPostEffects.Execute(m_HDRRenderTarget.get(), MemoryAccessFlags::FragmentAttachmentColor);

        cmd->Blit(m_HDRRenderTarget->GetColor(0), window, 0, 0, FilterMode::Bilinear);
    }

    void RenderPipeline::Step(AssetImportToken<ApplicationConfig>* token)
    {
        m_OEMTexture = token->assetDatabase->Load<Texture>(token->asset->FileBackgroundTexture.value.c_str());
        m_constantsPerFrame->Set<float>(HashCache::Get()->pk_SceneOEM_Exposure, token->asset->BackgroundExposure);
        m_passPostEffects.OnUpdateParameters(token->asset);
    }
}