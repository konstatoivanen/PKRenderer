#include "PrecompiledHeader.h"
#include "RenderPipeline.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/VulkanRHI/VulkanWindow.h"
#include "Rendering/VulkanRHI/Objects/VulkanBuffer.h"
#include "Rendering/MeshUtility.h"
#include "Utilities/Log.h"

namespace PK::Rendering
{
    using namespace PK::Rendering::Objects;

    struct Vertex
    {
        float3 position;
        float3 color;
        float2 texcoord;
    };

    struct UniformBufferObject
    {
        float4x4 model;
        float4x4 viewproj;
    };

    static const std::vector<Vertex> PK_DEBUG_VERTICES =
    {
        {{-0.5f, 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 1.0f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 1.0f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
    };

    static const std::vector<ushort> PK_DEBUG_INDICES =
    {
        0, 1, 2,
        2, 3, 0,

        4, 5, 6,
        6, 7, 4
    };

    RenderPipeline::RenderPipeline(AssetDatabase* assetDatabase, int width, int height)
    {
        auto driver = GraphicsAPI::GetActiveDriver<VulkanDriver>();

        m_shader = assetDatabase->Load<Shader>("res/SH_WS_Debug.pkshader");

        TextureDescriptor depthTexDescr{};
        depthTexDescr.resolution = { width, height, 1 };
        depthTexDescr.format = TextureFormat::Depth32F;
        depthTexDescr.usage = TextureUsage::RTDepth;
        m_depthTexture = CreateRef<VulkanTexture>(depthTexDescr);

        m_fixedFunctionState = {};
        m_fixedFunctionState.blending = m_shader->GetFixedFunctionAttributes().blending;
        m_fixedFunctionState.multisampling.sampleShadingEnable = VK_FALSE;
        m_fixedFunctionState.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        m_fixedFunctionState.multisampling.minSampleShading = 1.0f;
        m_fixedFunctionState.multisampling.alphaToCoverageEnable = VK_FALSE;
        m_fixedFunctionState.multisampling.alphaToOneEnable = VK_FALSE;
        m_fixedFunctionState.rasterization.depthClampEnable = VK_FALSE;
        m_fixedFunctionState.rasterization.rasterizerDiscardEnable = VK_FALSE;
        m_fixedFunctionState.rasterization.polygonMode = PolygonMode::Fill;
        m_fixedFunctionState.rasterization.lineWidth = 1.0f;
        m_fixedFunctionState.rasterization.cullMode = CullMode::Back;
        m_fixedFunctionState.rasterization.frontFace = FrontFace::CounterClockwise;
        m_fixedFunctionState.rasterization.depthBiasEnable = VK_FALSE;
        m_fixedFunctionState.rasterization.depthBiasConstantFactor = 0.0f;
        m_fixedFunctionState.rasterization.depthBiasClamp = 0.0f;
        m_fixedFunctionState.rasterization.depthBiasSlopeFactor = 0.0f;
        m_fixedFunctionState.depthStencil.depthWriteEnable = VK_TRUE;
        m_fixedFunctionState.depthStencil.depthCompareOp = Comparison::LessEqual;
        m_fixedFunctionState.colorTargetCount = 1;

        //m_mesh = MeshUtility::GetSphere(PK_FLOAT3_ZERO, 0.5f);

        m_uniformBuffer = Buffer::CreateUniform({ { ElementType::Float4x4, "model" }, { ElementType::Float4x4, "viewproj" }});

        m_mesh = assetDatabase->Load<Mesh>("res/MDL_Cloth.pkmesh");
        m_testTexture = assetDatabase->Load<Texture>("res/T_DebugTexture.ktx2");

        PK_LOG_VERBOSE("Render Pipeline Initialized!");
    }
    
    RenderPipeline::~RenderPipeline()
    {
        GraphicsAPI::GetActiveDriver<VulkanDriver>()->WaitForIdle();
        m_mesh = nullptr;
        m_uniformBuffer = nullptr;
        m_depthTexture = nullptr;
        m_shader = nullptr;
    }
    
    void RenderPipeline::Step(Window* window, int condition)
    {
        auto driver = GraphicsAPI::GetActiveDriver<VulkanDriver>();
        auto vkWindow = static_cast<VulkanWindow*>(window);

        const auto& cmd = driver->commandBufferPool->GetCurrent();

        auto view = Functions::GetMatrixInvTRS({ 0,0,-2 }, PK_FLOAT3_ZERO, PK_FLOAT3_ONE);
        auto proj = Functions::GetPerspective(90.0f, window->GetAspectRatio(), 0.1f, 100.0f);

        UniformBufferObject ubodata{};
        ubodata.model = Functions::GetMatrixTRS(PK_FLOAT3_ZERO, { 0, 0.01f * (m_rotation++), 0 }, PK_FLOAT3_ONE);
        ubodata.viewproj = proj * view;
        m_uniformBuffer->SetData(&ubodata, 0, sizeof(UniformBufferObject));

        m_depthTexture->Validate(window->GetResolution());

        cmd->SetRenderTarget(vkWindow->GetRenderTarget(), 0);
        cmd->SetRenderTarget(m_depthTexture->GetRenderTarget(), 0);
        cmd->ClearColor(PK_COLOR_RED, 0);
        cmd->ClearDepth(1.0f, 0u);

        cmd->BeginRenderPass();

        cmd->renderState.pipelineKey.fixedFunctionState = m_fixedFunctionState;

        auto vbuff = m_mesh->GetVertexBuffer(0)->GetNative<VulkanBuffer>()->GetBindHandle();
        auto ibuff = m_mesh->GetIndexBuffer()->GetNative<VulkanBuffer>()->GetBindHandle();
        auto ubuff = m_uniformBuffer->GetNative<VulkanBuffer>()->GetBindHandle();

        cmd->SetShader(m_shader->GetVariant(0)->GetNative<VulkanShader>());
        cmd->BindVertexBuffers({ { vbuff, InputRate::PerVertex } });
        cmd->BindIndexBuffer(ibuff, 0);

        cmd->BindResource("ubo", ubuff);
        cmd->BindResource("tex1", m_testTexture->GetNative<VulkanTexture>()->GetBindHandle());

        cmd->SetViewPort(vkWindow->GetRect(), 0.0f, 1.0f);
        cmd->SetScissor(vkWindow->GetRect());

        auto sm = m_mesh->GetSubmesh(0);
        cmd->DrawIndexed(sm.count, 1, sm.offset, 0, 0);

        cmd->EndRenderPass();
    }
}