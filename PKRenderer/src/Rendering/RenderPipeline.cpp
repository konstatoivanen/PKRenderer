#include "PrecompiledHeader.h"
#include "RenderPipeline.h"
#include "Rendering/MeshUtility.h"
#include "Utilities/Log.h"

namespace PK::Rendering
{
    using namespace PK::Rendering::Objects;

    struct UniformBufferObject
    {
        float4x4 model;
        float4x4 viewproj;
    };

    RenderPipeline::RenderPipeline(AssetDatabase* assetDatabase, int width, int height)
    {
        m_shader = assetDatabase->Load<Shader>("res/SH_WS_Debug.pkshader");

        RenderTextureDescriptor descriptor{};
        descriptor.resolution = { width, height, 1 };
        descriptor.colorFormats[0] = TextureFormat::RGBA16F;
        descriptor.depthFormat = TextureFormat::Depth32F;
        descriptor.usage = TextureUsage::Sample;
        m_renderTarget = CreateRef<RenderTexture>(descriptor);

        m_uniformBuffer = Buffer::CreateUniform({ { ElementType::Float4x4, "model" }, { ElementType::Float4x4, "viewproj" }});

        m_mesh = assetDatabase->Load<Mesh>("res/MDL_Cloth.pkmesh");
        m_testTexture = assetDatabase->Load<Texture>("res/T_DebugTexture.ktx2");

        PK_LOG_VERBOSE("Render Pipeline Initialized!");
    }
    
    RenderPipeline::~RenderPipeline()
    {
        GraphicsAPI::GetActiveDriver()->WaitForIdle();
        m_mesh = nullptr;
        m_uniformBuffer = nullptr;
        m_renderTarget = nullptr;
        m_shader = nullptr;
    }
    
    void RenderPipeline::Step(Window* window, int condition)
    {
        auto* cmd = GraphicsAPI::GetCommandBuffer();

        auto view = Functions::GetMatrixInvTRS({ 0,0,-2 }, PK_FLOAT3_ZERO, PK_FLOAT3_ONE);
        auto proj = Functions::GetPerspective(90.0f, window->GetAspectRatio(), 0.1f, 100.0f);

        auto uboView = m_uniformBuffer->BeginMap<UniformBufferObject>();
        uboView[0].model = Functions::GetMatrixTRS(PK_FLOAT3_ZERO, { 0, 0.01f * (m_rotation++), 0 }, PK_FLOAT3_ONE);
        uboView[0].viewproj = proj * view;
        m_uniformBuffer->EndMap();

        m_renderTarget->Validate(window->GetResolution());

        cmd->SetRenderTarget(m_renderTarget.get());
        cmd->ClearColor(PK_COLOR_RED, 0);
        cmd->ClearDepth(1.0f, 0u);

        cmd->BeginRenderPass();

        cmd->SetShader(m_shader, 0);
        cmd->SetMesh(m_mesh);

        cmd->SetBuffer("ubo", m_uniformBuffer.get());
        cmd->SetTexture("tex1", m_testTexture);
        cmd->SetConstant<float4>("offset", { 0, glm::sin(m_rotation * 0.01f), 0, 0 });

        cmd->SetViewPort(window->GetRect(), 0.0f, 1.0f);
        cmd->SetScissor(window->GetRect());

        cmd->DrawMesh(m_mesh, 0);

        cmd->EndRenderPass();

        cmd->Blit(m_renderTarget->GetColor(0), window, 0, 0, FilterMode::Point);
    }
}