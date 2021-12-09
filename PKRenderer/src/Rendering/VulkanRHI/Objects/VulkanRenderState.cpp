#include "PrecompiledHeader.h"
#include "VulkanRenderState.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Utilities/Log.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    void VulkanRenderState::Reset()
    {
        memset(descriptorSetKeys, 0, sizeof(descriptorSetKeys));
        memset(&pipelineKey, 0, sizeof(PipelineKey));
        memset(&frameBufferKey, 0, sizeof(FrameBufferKey));
        memset(&renderPassKey, 0, sizeof(RenderPassKey));
        memset(&descriptorsDirty, 0, sizeof(descriptorsDirty));

        pipelineKey.fixedFunctionState = FixedFunctionState();
        renderPass = nullptr;
        pipeline = nullptr;
        frameBuffer = nullptr;
        pipelineIsDirty = true;
        vertexBuffersDirty = true;

        frameBufferKey.layers = 1;
        renderPassKey.samples = 1;
        pipelineKey.primitiveRestart = VK_FALSE;
        pipelineKey.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    void VulkanRenderState::PrepareRenderPass()
    {
        renderPass = frameBufferCache->GetRenderPass(renderPassKey);
        frameBufferKey.renderPass = renderPass->renderPass;
        frameBuffer = frameBufferCache->GetFrameBuffer(frameBufferKey);   
        pipelineKey.renderPass = renderPass->renderPass;
        pipelineIsDirty = true;
        
        for (auto i = 0u; i < PK_MAX_RENDER_TARGETS; ++i)
        {
            if (renderPassKey.colors[i].format == VK_FORMAT_UNDEFINED)
            {
                pipelineKey.fixedFunctionState.colorTargetCount = i;
                clearValueCount = i;
                clearValues[i] = clearValues[PK_MAX_RENDER_TARGETS];
                break;
            }
        }

        if (renderPassKey.depth.format != VK_FORMAT_UNDEFINED)
        {
            clearValueCount++;
        }
    }

    void VulkanRenderState::SetRenderTarget(const VulkanRenderTarget& renderTarget, uint32_t index)
    {
        renderPassKey.samples = renderTarget.samples;
        frameBufferKey.layers = renderTarget.layers;
        frameBufferKey.extent = { renderTarget.extent.width, renderTarget.extent.height };

        renderArea = { {}, { renderTarget.extent.width, renderTarget.extent.height } };

        auto isDepth = EnumConvert::IsDepthFormat(renderTarget.format);
        auto attachment = isDepth ? &renderPassKey.depth : (renderPassKey.colors + index);
        attachment->format = renderTarget.format;
        attachment->layout = renderTarget.layout;
        attachment->loadop = LoadOp::Keep;
        attachment->storeop = StoreOp::Store;
        attachment->resolve = renderTarget.samples > 1;

        if (isDepth)
        {
            frameBufferKey.depth = renderTarget.view;
        }
        else
        {
            frameBufferKey.color[index] = renderTarget.view;
        }
    }

    void VulkanRenderState::SetResolveTarget(const VulkanRenderTarget& renderTarget, uint32_t index)
    {
        frameBufferKey.resolve[index] = renderTarget.view;
        renderPassKey.colors[index].resolve = true;
    }

    void VulkanRenderState::SetRenderArea(const VkRect2D& rect)
    {
        renderArea = rect;
    }
    
    void VulkanRenderState::ClearColor(const color& color, uint32_t index)
    {
        renderPassKey.colors[index].loadop = LoadOp::Clear;
        clearValues[index].color.float32[0] = color.r;
        clearValues[index].color.float32[1] = color.g;
        clearValues[index].color.float32[2] = color.b;
        clearValues[index].color.float32[3] = color.a;
    }
    
    void VulkanRenderState::ClearDepth(float depth, uint32_t stencil)
    {
        renderPassKey.depth.loadop = LoadOp::Clear;
        clearValues[PK_MAX_RENDER_TARGETS].depthStencil.depth = depth;
        clearValues[PK_MAX_RENDER_TARGETS].depthStencil.stencil = stencil;
    }
    
    void VulkanRenderState::DiscardColor(uint32_t index)
    {
        renderPassKey.colors[index].loadop = LoadOp::Discard;
    }

    void VulkanRenderState::DiscardDepth()
    {
        renderPassKey.depth.loadop = LoadOp::Discard;
    }

    void VulkanRenderState::SetShader(const VulkanShader* shader)
    {
        if (pipelineKey.shader == shader)
        {
            return;
        }

        pipelineIsDirty = true;
        pipelineKey.shader = shader;

        auto index = 0u;

        for (const auto& element : shader->GetVertexLayout())
        {
            auto* attribute = pipelineKey.vertexAttributes + index++;
            auto format = EnumConvert::GetFormat(element.Type);

            if (attribute->location != element.Location ||
                attribute->format != format)
            {
                vertexBuffersDirty = true;
                attribute->location = element.Location;
                attribute->format = format;
            }
        }

        for (; index < PK_MAX_VERTEX_ATTRIBUTES; ++index)
        {
            // Attribute count changed
            if (pipelineKey.vertexAttributes[index].format != VK_FORMAT_UNDEFINED)
            {
                vertexBuffersDirty = true;
                pipelineKey.vertexAttributes[index] = {};
            }
        }

        for (auto i = 0; i < PK_MAX_DESCRIPTOR_SETS; ++i)
        {
            if (shader->GetDescriptorSetLayout(i) == nullptr)
            {
                continue;
            }

            index = 0u;

            for (const auto& element : shader->GetResourceLayout(i))
            {
                auto* binding = descriptorSetKeys[i].bindings + index++;

                if (binding->binding != element.Binding ||
                    binding->count != element.Count ||
                    binding->type != element.Type)
                {
                    descriptorsDirty[i] = true;
                    binding->binding = element.Binding;
                    binding->count = element.Count;
                    binding->type = element.Type;
                }
            }

            for (; index < PK_MAX_DESCRIPTORS_PER_SET; ++index)
            {
                // Binding count changed
                if (descriptorSetKeys[i].bindings[index].count != 0)
                {
                    descriptorsDirty[i] = true;
                    descriptorSetKeys[i].bindings[index] = {};
                }
            }
        }
    }

    void VulkanRenderState::SetBlending(const BlendParameters& blend)
    {
        if (memcmp(&pipelineKey.fixedFunctionState.blending, &blend, sizeof(BlendParameters)) != 0)
        {
            pipelineKey.fixedFunctionState.blending = blend;
            pipelineIsDirty = true;
        }
    }

    void VulkanRenderState::SetRasterization(const RasterizationParameters& rasterization)
    {
        if (memcmp(&pipelineKey.fixedFunctionState.rasterization, &rasterization, sizeof(RasterizationParameters)) != 0)
        {
            pipelineKey.fixedFunctionState.rasterization = rasterization;
            pipelineIsDirty = true;
        }
    }

    void VulkanRenderState::SetDepthStencil(const DepthStencilParameters& depthStencil)
    {
        if (memcmp(&pipelineKey.fixedFunctionState.depthStencil, &depthStencil, sizeof(DepthStencilParameters)) != 0)
        {
            pipelineKey.fixedFunctionState.depthStencil = depthStencil;
            pipelineIsDirty = true;
        }
    }

    void VulkanRenderState::SetMultisampling(const MultisamplingParameters& multisampling)
    {
        if (memcmp(&pipelineKey.fixedFunctionState.multisampling, &multisampling, sizeof(MultisamplingParameters)) != 0)
        {
            pipelineKey.fixedFunctionState.multisampling = multisampling;
            pipelineIsDirty = true;
        }
    }

    void VulkanRenderState::SetResource(uint32_t nameHashId, const VulkanBindHandle* handle)
    {
        if (pipelineKey.shader == nullptr)
        {
            return;
        }

        for (auto i = 0; i < PK_MAX_DESCRIPTOR_SETS; ++i)
        {
            uint32_t index;
            
            if (!pipelineKey.shader->GetResourceLayout(i).TryGetElement(nameHashId, &index))
            {
                continue;
            }

            if (descriptorSetKeys[i].bindings[index].handle != handle)
            {
                descriptorSetKeys[i].bindings[index].handle = handle;
                descriptorsDirty[i] = true;
            }
        }
    }

    void VulkanRenderState::SetVertexBuffers(const VulkanBindHandle** handles, uint32_t count)
    {
        auto i = 0u;

        for (; i < PK_MAX_VERTEX_ATTRIBUTES && i < count; ++i)
        {
            auto handle = handles[i];

            PK_THROW_ASSERT(handle != nullptr, "Passing null vertex buffer is not allowed!");

            if (handle != vertexBuffers[i])
            {
                vertexBuffers[i] = handle;
                vertexBuffersRaw[i] = handle->buffer;
                vertexBuffersDirty = true;
            }
        }

        vertexBufferBindCount = i;

        if (i < PK_MAX_VERTEX_ATTRIBUTES)
        {
            memset(vertexBuffers + i, 0, sizeof(vertexBuffers[0]) * (PK_MAX_VERTEX_ATTRIBUTES - i));
            memset(vertexBuffersRaw + i, 0, sizeof(vertexBuffersRaw[0]) * (PK_MAX_VERTEX_ATTRIBUTES - i));
        }
    }

    const VulkanPipeline* VulkanRenderState::GetComputePipeline(const VulkanShader* shader)
    {
        pipelineIsDirty = true;
        return pipelineCache->GetComputePipeline(shader);
    }

    PKRenderStateDirtyFlags VulkanRenderState::ValidatePipeline(const VulkanExecutionGate& gate)
    {
        PK_THROW_ASSERT(pipelineKey.shader != nullptr, "Pipeline validation failed! Shader is unassigned!");

        uint32_t flags = 0u;

        if (vertexBuffersDirty)
        {
            flags |= PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;

            const auto& vertexLayout = pipelineKey.shader->GetVertexLayout();

            for (auto i = 0u; i < PK_MAX_VERTEX_ATTRIBUTES; ++i)
            {
                auto vertexBuffer = vertexBuffers[i];
                auto bufferAttribute = &pipelineKey.vertexBuffers[i];
                
                if (vertexBuffer == nullptr)
                {
                    // Buffer count changed!
                    if (bufferAttribute->stride != 0)
                    {
                        pipelineIsDirty = true;
                    }

                    *bufferAttribute = {};
                    break;
                }

                const auto& bufferLayout = *vertexBuffer->bufferLayout;
                auto stride = bufferLayout.GetStride();
                
                bufferAttribute->binding = i;
            
                if (bufferAttribute->inputRate != vertexBuffer->inputRate || bufferAttribute->stride != stride)
                {
                    bufferAttribute->inputRate = vertexBuffer->inputRate;
                    bufferAttribute->stride = stride;
                    pipelineIsDirty = true;
                }

                for (const auto& element : bufferLayout)
                {
                    uint32_t elementIdx = 0u;
                    auto* velement = vertexLayout.TryGetElement(element.NameHashId, &elementIdx);

                    if (velement == nullptr)
                    {
                        continue;
                    }

                    auto* attribute = &pipelineKey.vertexAttributes[elementIdx];

                    if (attribute->format == EnumConvert::GetFormat(element.Type) && (attribute->binding != i || attribute->offset != element.Offset))
                    {
                        pipelineIsDirty = true;
                        attribute->binding = i;
                        attribute->offset = element.Offset;
                    }
                }
            }

            vertexBuffersDirty = false;
        }

        for (auto i = 0; i < PK_MAX_DESCRIPTOR_SETS; ++i)
        {
            if (descriptorsDirty[i])
            {
                flags |= (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i);
                descriptorSets[i] = descriptorCache->GetDescriptorSet(pipelineKey.shader->GetDescriptorSetLayout(i), descriptorSetKeys[i], gate);
                descriptorsDirty[i] = false;
            }
        }

        if (pipelineIsDirty)
        {
            flags |= PK_RENDER_STATE_DIRTY_PIPELINE;
            pipeline = pipelineCache->GetGraphicsPipeline(pipelineKey);
            pipelineIsDirty = false;
        }

        return (PKRenderStateDirtyFlags)flags;
    }
}