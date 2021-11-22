#include "PrecompiledHeader.h"
#include "RenderPipeline.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/VulkanRHI/VulkanWindow.h"
#include "Utilities/Log.h"
#include <shaderc/shaderc.hpp>

namespace PK::Rendering
{
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

    static std::vector<uint32_t> CompileGLSLToSpirV(const std::string& source_name, shaderc_shader_kind kind, const std::string& source, bool optimize = false)
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Like -DMY_DEFINE=1
        //options.AddMacroDefinition("MY_DEFINE", "1");

        if (optimize)
        {
            //options.SetOptimizationLevel(shaderc_optimization_level_size);
        }

        shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

        if (module.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            throw std::runtime_error(module.GetErrorMessage());
        }

        return { module.cbegin(), module.cend() };
    }

    RenderPipeline::RenderPipeline(int width, int height)
    {
        auto driver = GraphicsAPI::GetActiveDriver<VulkanDriver>();

        VulkanShaderCreateInfo shaderInfo{};
        shaderInfo.spirv[(int)ShaderStage::Vertex] = CompileGLSLToSpirV("DEBUG VERTEX", shaderc_shader_kind::shaderc_vertex_shader, std::string(PK_DEBUG_VERTEX_SHADER), true);
        shaderInfo.spirv[(int)ShaderStage::Fragment] = CompileGLSLToSpirV("DEBUG FRAGMENT", shaderc_shader_kind::shaderc_fragment_shader, std::string(PK_DEBUG_FRAGMENT_SHADER), true);
        m_shader = CreateRef<VulkanShader>(driver->device, shaderInfo);

        TextureDescriptor depthTexDescr{};
        depthTexDescr.resolution = { width, height, 1 };
        depthTexDescr.format = TextureFormat::Depth32F;
        depthTexDescr.usage = TextureUsage::RTDepth;
        m_depthTexture = CreateRef<VulkanTexture>(driver, depthTexDescr);

        m_fixedFunctionState = {};
        m_fixedFunctionState.blending.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        m_fixedFunctionState.blending.blendEnable = VK_FALSE;
        m_fixedFunctionState.blending.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        m_fixedFunctionState.blending.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        m_fixedFunctionState.blending.colorBlendOp = VK_BLEND_OP_ADD;
        m_fixedFunctionState.blending.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        m_fixedFunctionState.blending.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        m_fixedFunctionState.blending.alphaBlendOp = VK_BLEND_OP_ADD;
        m_fixedFunctionState.multisampling.sampleShadingEnable = VK_FALSE;
        m_fixedFunctionState.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        m_fixedFunctionState.multisampling.minSampleShading = 1.0f;
        m_fixedFunctionState.multisampling.alphaToCoverageEnable = VK_FALSE;
        m_fixedFunctionState.multisampling.alphaToOneEnable = VK_FALSE;
        m_fixedFunctionState.rasterization.depthClampEnable = VK_FALSE;
        m_fixedFunctionState.rasterization.rasterizerDiscardEnable = VK_FALSE;
        m_fixedFunctionState.rasterization.polygonMode = VK_POLYGON_MODE_FILL;
        m_fixedFunctionState.rasterization.lineWidth = 1.0f;
        m_fixedFunctionState.rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
        m_fixedFunctionState.rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
        m_fixedFunctionState.rasterization.depthBiasEnable = VK_FALSE;
        m_fixedFunctionState.rasterization.depthBiasConstantFactor = 0.0f;
        m_fixedFunctionState.rasterization.depthBiasClamp = 0.0f;
        m_fixedFunctionState.rasterization.depthBiasSlopeFactor = 0.0f;
        m_fixedFunctionState.depthStencil.depthTestEnable = VK_TRUE;
        m_fixedFunctionState.depthStencil.depthWriteEnable = VK_TRUE;
        m_fixedFunctionState.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        m_fixedFunctionState.colorTargetCount = 1;

        m_vertexBuffer = CreateRef<VulkanBuffer>(driver, BufferUsage::Vertex,
                                                    BufferLayout
                                                    ({
                                                        { ElementType::Float3, "inPosition" },
                                                        { ElementType::Float3, "inColor" },
                                                        { ElementType::Float2, "inTexcoord" },
                                                    }),
                                                    PK_DEBUG_VERTICES.size());

        m_indexBuffer = CreateRef<VulkanBuffer>(driver, BufferUsage::Index,
                                                    BufferLayout
                                                    ({
                                                         { ElementType::Ushort, "INDEX" }
                                                    }),
                                                    PK_DEBUG_INDICES.size());

        m_uniformBuffer = CreateRef<VulkanBuffer>(driver, BufferUsage::Uniform,
                                                    BufferLayout
                                                    ({
                                                         { ElementType::Float4x4, "model" },
                                                         { ElementType::Float4x4, "viewproj" },
                                                    }), 
                                                    1);


        m_vertexBuffer->SetData(PK_DEBUG_VERTICES.data(), 0, m_vertexBuffer->GetCapacity());
        m_indexBuffer->SetData(PK_DEBUG_INDICES.data(), 0, m_indexBuffer->GetCapacity());

        m_vulkanTexture = CreateRef<VulkanTexture>(driver, "res/T_DebugTexture.ktx2");

        PK_LOG_VERBOSE("Render Pipeline Initialized!");
    }
    
    RenderPipeline::~RenderPipeline()
    {
        GraphicsAPI::GetActiveDriver<VulkanDriver>()->WaitForIdle();
        m_vertexBuffer = nullptr;
        m_indexBuffer = nullptr;
        m_uniformBuffer = nullptr;
        m_vulkanTexture = nullptr;
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

        cmd->SetShader(m_shader.get());
        cmd->BindVertexBuffers({ { m_vertexBuffer->GetBindHandle(), InputRate::PerVertex } });
        cmd->BindIndexBuffer(m_indexBuffer->GetBindHandle(), 0);

        cmd->BindResource("ubo", m_uniformBuffer->GetBindHandle());
        cmd->BindResource("tex1", m_vulkanTexture->GetBindHandle());

        cmd->SetViewPort(vkWindow->GetRect(), 0.0f, 1.0f);
        cmd->SetScissor(vkWindow->GetRect());

        cmd->DrawIndexed(static_cast<uint32_t>(PK_DEBUG_INDICES.size()), 1, 0, 0, 0);

        cmd->EndRenderPass();
    }
}