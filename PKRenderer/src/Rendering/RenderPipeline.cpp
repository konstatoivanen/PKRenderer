#include "PrecompiledHeader.h"
#include "RenderPipeline.h"
#include "Rendering/MeshUtility.h"
#include "Utilities/Log.h"

namespace PK::Rendering
{
    using namespace PK::Rendering::Objects;

    struct PerFrameConstants
    {
        float4 pk_Time;
        float4 pk_SinTime;
        float4 pk_CosTime;
        float4 pk_DeltaTime;
        float4 pk_CursorParams;
        float4 pk_WorldSpaceCameraPos;
        float4 pk_ProjectionParams;
        float4 pk_ExpProjectionParams;
        float4 pk_ScreenParams;
        float4 pk_ShadowCascadeZSplits;
        float4x4 pk_MATRIX_V;
        float4x4 pk_MATRIX_I_V;
        float4x4 pk_MATRIX_P;
        float4x4 pk_MATRIX_I_P;
        float4x4 pk_MATRIX_VP;
        float4x4 pk_MATRIX_I_VP;
        float4x4 pk_MATRIX_L_VP;
        float pk_SceneOEM_Exposure;
    };

    struct ModelMatrices
    {
        float4x4 pk_MATRIX_M;
        float4x4 pk_MATRIX_I_M;
    };

    static void SetViewProjectionMatrices(PerFrameConstants* buffer, const float4x4& view, const float4x4& projection)
    {
        auto cameraMatrix = glm::inverse(view);

        auto n = Functions::GetZNearFromProj(projection);
        auto f = Functions::GetZFarFromProj(projection);
        auto vp = projection * view;
        auto pvp = buffer->pk_MATRIX_L_VP;

        buffer->pk_ProjectionParams = { n, f, f - n, 1.0f / f };
        buffer->pk_ExpProjectionParams = { 1.0f / glm::log2(f / n), -log2(n) / log2(f / n), f / n, 1.0f / n };
        buffer->pk_WorldSpaceCameraPos = cameraMatrix[3];
        buffer->pk_MATRIX_V = view;
        buffer->pk_MATRIX_I_V = cameraMatrix;
        buffer->pk_MATRIX_P = projection;
        buffer->pk_MATRIX_I_P = glm::inverse(projection);
        buffer->pk_MATRIX_VP = vp;
        buffer->pk_MATRIX_I_VP = glm::inverse(vp);
        buffer->pk_MATRIX_L_VP = pvp;
    }

    RenderPipeline::RenderPipeline(AssetDatabase* assetDatabase, int width, int height)
    {
        m_shader = assetDatabase->Load<Shader>("res/shaders/SH_WS_Debug.pkshader");

        RenderTextureDescriptor descriptor{};
        descriptor.resolution = { width, height, 1 };
        descriptor.colorFormats[0] = TextureFormat::RGBA16F;
        descriptor.depthFormat = TextureFormat::Depth32F;
        descriptor.usage = TextureUsage::Sample;
        m_renderTarget = CreateRef<RenderTexture>(descriptor);

        m_perFrameConstants = Buffer::CreateUniform
        (
            { 
                { ElementType::Float4x4, "model" },
                { ElementType::Float4x4, "viewproj" },
                { ElementType::Float4, "pk_Time" },
                { ElementType::Float4, "pk_SinTime" },
                { ElementType::Float4, "pk_CosTime" },
                { ElementType::Float4, "pk_DeltaTime" },
                { ElementType::Float4, "pk_CursorParams" },
                { ElementType::Float4, "pk_WorldSpaceCameraPos" },
                { ElementType::Float4, "pk_ProjectionParams" },
                { ElementType::Float4, "pk_ExpProjectionParams" },
                { ElementType::Float4, "pk_ScreenParams" },
                { ElementType::Float4, "pk_ShadowCascadeZSplits" },
                { ElementType::Float4x4, "pk_MATRIX_V" },
                { ElementType::Float4x4, "pk_MATRIX_I_V" },
                { ElementType::Float4x4, "pk_MATRIX_P" },
                { ElementType::Float4x4, "pk_MATRIX_I_P" },
                { ElementType::Float4x4, "pk_MATRIX_VP" },
                { ElementType::Float4x4, "pk_MATRIX_I_VP" },
                { ElementType::Float4x4, "pk_MATRIX_L_VP" },
                { ElementType::Float, "pk_SceneOEM_Exposure" }
            }
        );

        m_modelMatrices = Buffer::CreateUniform
        (
            {
                { ElementType::Float4x4, "pk_MATRIX_M" },
                { ElementType::Float4x4, "pk_MATRIX_I_M" },
            }
        );

        m_mesh = assetDatabase->Load<Mesh>("res/models/MDL_Cloth.pkmesh");
        m_testTexture = assetDatabase->Load<Texture>("res/textures/T_DebugTexture.ktx2");

        PK_LOG_VERBOSE("Render Pipeline Initialized!");
    }
    
    RenderPipeline::~RenderPipeline()
    {
        GraphicsAPI::GetActiveDriver()->WaitForIdle();
        m_mesh = nullptr;
        m_perFrameConstants = nullptr;
        m_renderTarget = nullptr;
        m_shader = nullptr;
    }
    
    void RenderPipeline::Step(Window* window, int condition)
    {
        auto* cmd = GraphicsAPI::GetCommandBuffer();

        auto perFrameView = m_perFrameConstants->BeginMap<PerFrameConstants>();
        SetViewProjectionMatrices(perFrameView.data, Functions::GetMatrixInvTRS({ 0,0,-2 }, PK_FLOAT3_ZERO, PK_FLOAT3_ONE), Functions::GetPerspective(90.0f, window->GetAspectRatio(), 0.1f, 100.0f));
        m_perFrameConstants->EndMap();

        auto modelView = m_modelMatrices->BeginMap<ModelMatrices>();
        modelView[0].pk_MATRIX_M = Functions::GetMatrixTRS(PK_FLOAT3_ZERO, { 0, 0.01f * (m_rotation++), 0 }, PK_FLOAT3_ONE);
        modelView[0].pk_MATRIX_I_M = glm::inverse(modelView[0].pk_MATRIX_M);
        m_modelMatrices->EndMap();

        m_renderTarget->Validate(window->GetResolution());

        cmd->SetRenderTarget(m_renderTarget.get());
        cmd->ClearColor(PK_COLOR_RED, 0);
        cmd->ClearDepth(1.0f, 0u);

        cmd->BeginRenderPass();

        cmd->SetBuffer("pk_PerFrameConstants", m_perFrameConstants.get());
        cmd->SetBuffer("pk_ModelMatrices", m_modelMatrices.get());
        cmd->SetTexture("tex1", m_testTexture);
        cmd->SetConstant<float4>("offset", { 0, glm::sin(m_rotation * 0.01f), 0, 0 });

        cmd->SetViewPort(window->GetRect(), 0.0f, 1.0f);
        cmd->SetScissor(window->GetRect());

        cmd->DrawMesh(m_mesh, 0, m_shader);

        cmd->EndRenderPass();

        cmd->Blit(m_renderTarget->GetColor(0), window, 0, 0, FilterMode::Point);
    }
}