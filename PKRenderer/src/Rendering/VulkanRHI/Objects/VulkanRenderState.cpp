#include "PrecompiledHeader.h"
#include "VulkanRenderState.h"
#include "Core/Services/Log.h"
#include "Utilities/Handle.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/VulkanRHI/Objects/VulkanBindArray.h"
#include "Rendering/GraphicsAPI.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    void VulkanRenderState::Reset()
    {
        memset(m_descriptorSetKeys, 0, sizeof(m_descriptorSetKeys));
        memset(&m_pipelineKey, 0, sizeof(PipelineKey));
        memset(m_frameBufferKey, 0, sizeof(FrameBufferKey) * 2);
        memset(m_renderPassKey, 0, sizeof(RenderPassKey) * 2);

        m_pipelineKey.fixedFunctionState = FixedFunctionState();
        m_renderPass = nullptr;
        m_pipeline = nullptr;
        m_frameBuffer = nullptr;
        m_dirtyFlags = PK_RENDER_STATE_DIRTY_RENDERTARGET | PK_RENDER_STATE_DIRTY_PIPELINE | PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;

        m_frameBufferKey[0].layers = 1;
        m_frameBufferKey[1].layers = 1;
        m_renderPassKey[0].samples = 1;
        m_renderPassKey[1].samples = 1;
        m_pipelineKey.primitiveRestart = VK_FALSE;
        m_pipelineKey.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    void VulkanRenderState::SetRenderTarget(const VulkanRenderTarget* renderTargets, const VulkanRenderTarget* resolves, uint32_t count)
    {
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;

        auto passKey = m_renderPassKey;
        auto fboKey = m_frameBufferKey;

        memset(passKey, 0, sizeof(RenderPassKey));
        memset(fboKey, 0, sizeof(FrameBufferKey));

        // These should be the same for all targets. Validation will assert if not.
        passKey->samples = renderTargets[0].samples;
        fboKey->layers = renderTargets[0].layers;
        fboKey->extent = { renderTargets[0].extent.width, renderTargets[0].extent.height };
        m_renderArea = { {}, fboKey->extent };

        for (auto i = 0u, j = 0u; i < count; ++i)
        {
            auto target = renderTargets + i;
            auto resolve = resolves != nullptr ? resolves + i : nullptr;

            auto isDepth = EnumConvert::IsDepthFormat(target->format);
            auto attachment = isDepth ? &passKey->depth : (passKey->colors + j);
            attachment->format = target->format;
            attachment->layout = target->layout;
            attachment->loadop = LoadOp::Keep;
            attachment->storeop = StoreOp::Store;
            attachment->resolve = false;

            if (isDepth)
            {
                // @TODO Handle depth resolves
                fboKey->depth = target->view;
            }
            else
            {
                attachment->resolve = resolve != nullptr && resolve->view != VK_NULL_HANDLE;

                if (attachment->resolve)
                {
                    fboKey->resolve[j] = resolve->view;
                }

                fboKey->color[j++] = target->view;
            }
        }
    }

    void VulkanRenderState::SetRenderArea(const VkRect2D& rect)
    {
        m_renderArea = rect;
    }
    
    void VulkanRenderState::ClearColor(const color& color, uint32_t index)
    {
        m_renderPassKey[0].colors[index].loadop = LoadOp::Clear;
        m_clearValues[index].color.float32[0] = color.r;
        m_clearValues[index].color.float32[1] = color.g;
        m_clearValues[index].color.float32[2] = color.b;
        m_clearValues[index].color.float32[3] = color.a;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
    }
    
    void VulkanRenderState::ClearDepth(float depth, uint32_t stencil)
    {
        m_renderPassKey[0].depth.loadop = LoadOp::Clear;
        m_clearValues[PK_MAX_RENDER_TARGETS].depthStencil.depth = depth;
        m_clearValues[PK_MAX_RENDER_TARGETS].depthStencil.stencil = stencil;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
    }
    
    void VulkanRenderState::DiscardColor(uint32_t index)
    {
        m_renderPassKey[0].colors[index].loadop = LoadOp::Discard;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
    }

    void VulkanRenderState::DiscardDepth()
    {
        m_renderPassKey[0].depth.loadop = LoadOp::Discard;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
    }

    void VulkanRenderState::SetShader(const VulkanShader* shader)
    {
        if (m_pipelineKey.shader != shader)
        {
            m_pipelineKey.shader = shader;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE | PK_RENDER_STATE_DIRTY_SHADER;
        }
    }

    void VulkanRenderState::SetBlending(const BlendParameters& blend)
    {
        if (memcmp(&m_pipelineKey.fixedFunctionState.blending, &blend, sizeof(BlendParameters)) != 0)
        {
            m_pipelineKey.fixedFunctionState.blending = blend;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
        }
    }

    void VulkanRenderState::SetRasterization(const RasterizationParameters& rasterization)
    {
        if (memcmp(&m_pipelineKey.fixedFunctionState.rasterization, &rasterization, sizeof(RasterizationParameters)) != 0)
        {
            m_pipelineKey.fixedFunctionState.rasterization = rasterization;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
        }
    }

    void VulkanRenderState::SetDepthStencil(const DepthStencilParameters& depthStencil)
    {
        if (memcmp(&m_pipelineKey.fixedFunctionState.depthStencil, &depthStencil, sizeof(DepthStencilParameters)) != 0)
        {
            m_pipelineKey.fixedFunctionState.depthStencil = depthStencil;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
        }
    }

    void VulkanRenderState::SetMultisampling(const MultisamplingParameters& multisampling)
    {
        if (memcmp(&m_pipelineKey.fixedFunctionState.multisampling, &multisampling, sizeof(MultisamplingParameters)) != 0)
        {
            m_pipelineKey.fixedFunctionState.multisampling = multisampling;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
        }
    }

    void VulkanRenderState::SetVertexBuffers(const VulkanBindHandle** handles, uint32_t count)
    {
        auto i = 0u;

        for (; i < PK_MAX_VERTEX_ATTRIBUTES && i < count; ++i)
        {
            auto handle = handles[i];

            PK_THROW_ASSERT(handle != nullptr, "Passing null vertex buffer is not allowed!");

            if (handle != m_vertexBuffers[i])
            {
                m_vertexBuffers[i] = handle;
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;
            }
        }

        if (i < PK_MAX_VERTEX_ATTRIBUTES)
        {
            memset(m_vertexBuffers + i, 0, sizeof(m_vertexBuffers[0]) * (PK_MAX_VERTEX_ATTRIBUTES - i));
        }
    }


    VulkanVertexBufferBundle VulkanRenderState::GetVertexBufferBundle()
    {
        VulkanVertexBufferBundle bundle{};

        auto i = 0u;

        for (; i < PK_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            auto bufferAttrib = &m_pipelineKey.vertexBuffers[i];
            auto handle = m_vertexBuffers[i];

            if (bufferAttrib->stride == 0 || handle == nullptr)
            {
                break;
            }

            bundle.buffers[i] = handle->buffer;
        }

        bundle.count = i;

        return bundle;
    }

    VkRenderPassBeginInfo VulkanRenderState::GetRenderPassInfo()
    {
        VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        renderPassInfo.renderPass = m_renderPass->renderPass;
        renderPassInfo.framebuffer = m_frameBuffer->frameBuffer;
        renderPassInfo.renderArea = m_renderArea;
        renderPassInfo.clearValueCount = m_clearValueCount;
        renderPassInfo.pClearValues = m_clearValues;
        return renderPassInfo;
    }


    void VulkanRenderState::ValidateRenderTarget()
    {
        auto shader = m_pipelineKey.shader;
        auto shaderType = shader->GetType();

        if (shaderType != ShaderType::Graphics)
        {
            return;
        }

        if ((m_dirtyFlags & PK_RENDER_STATE_DIRTY_RENDERTARGET) == 0)
        {
            return;
        }

        if (memcmp(m_frameBufferKey, m_frameBufferKey + 1, sizeof(FrameBufferKey)) == 0 &&
            memcmp(m_renderPassKey, m_renderPassKey + 1, sizeof(RenderPassKey)) == 0)
        {
            m_dirtyFlags &= ~PK_RENDER_STATE_DIRTY_RENDERTARGET;
            return;
        }

        memcpy(m_frameBufferKey + 1, m_frameBufferKey, sizeof(FrameBufferKey));
        memcpy(m_renderPassKey + 1, m_renderPassKey, sizeof(RenderPassKey));

        m_renderPass = m_frameBufferCache->GetRenderPass(m_renderPassKey[0]);
        m_frameBufferKey[0].renderPass = m_renderPass->renderPass;
        m_frameBuffer = m_frameBufferCache->GetFrameBuffer(m_frameBufferKey[0]);

        m_pipelineKey.renderPass = m_renderPass->renderPass;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;

        for (auto i = 0u; i < PK_MAX_RENDER_TARGETS; ++i)
        {
            if (m_renderPassKey[0].colors[i].format == VK_FORMAT_UNDEFINED)
            {
                m_pipelineKey.fixedFunctionState.colorTargetCount = i;
                m_clearValueCount = i;
                m_clearValues[i] = m_clearValues[PK_MAX_RENDER_TARGETS];
                break;
            }
        }

        if (m_renderPassKey[0].depth.format != VK_FORMAT_UNDEFINED)
        {
            m_clearValueCount++;
        }
    }

    void VulkanRenderState::ValidateVertexBuffers()
    {
        auto shader = m_pipelineKey.shader;
        auto shaderType = shader->GetType();

        if (shaderType != ShaderType::Graphics)
        {
            return;
        }

        auto index = 0u;

        if (m_dirtyFlags & PK_RENDER_STATE_DIRTY_SHADER)
        {
            for (const auto& element : shader->GetVertexLayout())
            {
                auto* attribute = m_pipelineKey.vertexAttributes + index++;
                auto format = EnumConvert::GetFormat(element.Type);

                if (attribute->location != element.Location ||
                    attribute->format != format)
                {
                    m_dirtyFlags |= PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;
                    attribute->location = element.Location;
                    attribute->format = format;
                }
            }

            // Attribute count changed
            if (index < PK_MAX_VERTEX_ATTRIBUTES && m_pipelineKey.vertexAttributes[index].format != VK_FORMAT_UNDEFINED)
            {
                memset(m_pipelineKey.vertexAttributes + index, 0, sizeof(VkVertexInputAttributeDescription) * (PK_MAX_VERTEX_ATTRIBUTES - index));
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;
            }
        }

        if ((m_dirtyFlags & PK_RENDER_STATE_DIRTY_VERTEXBUFFERS) == 0)
        {
            return;
        }

        const auto& vertexLayout = shader->GetVertexLayout();

        for (index = 0u; index < PK_MAX_VERTEX_ATTRIBUTES; ++index)
        {
            auto vertexBuffer = m_vertexBuffers[index];
            auto bufferAttribute = &m_pipelineKey.vertexBuffers[index];
            
            if (vertexBuffer == nullptr)
            {
                break;
            }

            const auto& bufferLayout = *vertexBuffer->bufferLayout;
            auto stride = bufferLayout.GetStride();
            
            bufferAttribute->binding = index;
        
            if (bufferAttribute->inputRate != vertexBuffer->inputRate || bufferAttribute->stride != stride)
            {
                bufferAttribute->inputRate = vertexBuffer->inputRate;
                bufferAttribute->stride = stride;
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
            }

            for (const auto& element : bufferLayout)
            {
                uint32_t elementIdx = 0u;
                auto* velement = vertexLayout.TryGetElement(element.NameHashId, &elementIdx);

                if (velement == nullptr)
                {
                    continue;
                }

                auto* attribute = &m_pipelineKey.vertexAttributes[elementIdx];

                // Format equality might be too strict. disabling for now
                // @TODO Review this again later?
                if (/*attribute->format == EnumConvert::GetFormat(element.Type) && */ (attribute->binding != index || attribute->offset != element.Offset))
                {
                    m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
                    attribute->binding = index;
                    attribute->offset = element.Offset;
                }
            }
        }

        // Buffer count changed!
        if (index < PK_MAX_VERTEX_ATTRIBUTES && m_pipelineKey.vertexBuffers[index].stride != 0)
        {
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
            memset(m_pipelineKey.vertexBuffers + index, 0, sizeof(VkVertexInputBindingDescription) * (PK_MAX_VERTEX_ATTRIBUTES - index));
        }
    }

    void VulkanRenderState::ValidateDescriptorSets(const VulkanExecutionGate& gate)
    {
        auto shader = m_pipelineKey.shader;
        auto setCount = m_pipelineKey.shader->GetDescriptorSetCount();
        auto index = 0u;

        for (auto i = 0u; i < setCount; ++i)
        {
            auto layout = shader->GetDescriptorSetLayout(i);

            if (layout == nullptr)
            {
                continue;
            }

            if (m_descriptorSetKeys[i].stageFlags != layout->stageFlags)
            {
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i;
                m_descriptorSetKeys[i].stageFlags = layout->stageFlags;
            }

            auto* bindings = m_descriptorSetKeys[i].bindings;
            Handle<VulkanBindHandle> wrappedHandle = nullptr;
            Handle<VulkanBindArray> wrappedHandleArray = nullptr;
            index = 0u;

            for (const auto& element : shader->GetResourceLayout(i))
            {
                auto* binding = bindings + index++;
                
                if (element.Count > 1)
                {
                    PK_THROW_ASSERT(m_resourceProperties.TryGet(element.NameHashId, wrappedHandleArray), "Descriptors (%s) not bound!", StringHashID::IDToString(element.NameHashId).c_str());

                    uint32_t version = 0u;
                    uint32_t count = 0u;
                    auto handles = wrappedHandleArray.handle->GetHandles(&version, &count);
                    count = count < element.Count ? (uint16_t)count : element.Count;

                    if (binding->count != count || binding->type != element.Type || binding->handles != handles || binding->version != version || !binding->isArray)
                    {
                        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i;
                        binding->count = count;
                        binding->type = element.Type;
                        binding->handles = handles;
                        binding->version = version;
                        binding->isArray = true;
                    }

                    continue;
                }

                PK_THROW_ASSERT(m_resourceProperties.TryGet(element.NameHashId, wrappedHandle), "Descriptor (%s) not bound!", StringHashID::IDToString(element.NameHashId).c_str());
                auto handle = wrappedHandle.handle;

                if (binding->count != element.Count || binding->type != element.Type || binding->handle != handle || binding->version != handle->version || binding->isArray)
                {
                    m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i;
                    binding->count = element.Count;
                    binding->type = element.Type;
                    binding->handle = handle;
                    binding->version = handle->version;
                    binding->isArray = false;
                }
            }

            // Binding count changed
            if (index < PK_MAX_DESCRIPTORS_PER_SET && bindings[index].count != 0)
            {
                memset(bindings + index, 0, sizeof(bindings[0]) * (PK_MAX_DESCRIPTORS_PER_SET - index));
                m_dirtyFlags |= (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i);
            }

            if (m_dirtyFlags & (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i))
            {
                m_descriptorSets[i] = m_descriptorCache->GetDescriptorSet(shader->GetDescriptorSetLayout(i), m_descriptorSetKeys[i], gate);
            }

            m_dirtyFlags |= (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i);
        }

        // Clear remaining keys as they will go unbound when this pipe is used
        if (setCount < PK_MAX_DESCRIPTOR_SETS)
        {
            memset(m_descriptorSetKeys + setCount, 0, sizeof(m_descriptorSetKeys[0]) * (PK_MAX_DESCRIPTOR_SETS - setCount));
        }
    }

    PKRenderStateDirtyFlags VulkanRenderState::ValidatePipeline(const VulkanExecutionGate& gate)
    {
        PK_THROW_ASSERT(m_pipelineKey.shader != nullptr, "Pipeline validation failed! Shader is unassigned!");

        auto shaderType = m_pipelineKey.shader->GetType();

        // Lets not update vertex buffers for a compute pipeline
        if (shaderType == ShaderType::Compute)
        {
            m_dirtyFlags &= ~PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;
        }

        ValidateRenderTarget();
        ValidateVertexBuffers();
        ValidateDescriptorSets(gate);

        if (m_dirtyFlags & PK_RENDER_STATE_DIRTY_PIPELINE)
        {
            m_pipeline = m_pipelineCache->GetPipeline(m_pipelineKey);
        }

        auto flags = (PKRenderStateDirtyFlags)m_dirtyFlags;
        m_dirtyFlags = 0u;

        // Dont dirty render pass when not using a graphics pipeline
        if (shaderType != ShaderType::Graphics)
        {
            flags = (PKRenderStateDirtyFlags)(flags & ~PK_RENDER_STATE_DIRTY_RENDERTARGET);
        }
        
        return flags;
    }
}