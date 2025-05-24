#include "PrecompiledHeader.h"
#include "Core/Utilities/Handle.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanBindArray.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanRenderState.h"

namespace PK
{
    VkRenderingInfo VulkanRenderState::GetRenderPassInfo() const
    {
        static VkRenderingAttachmentInfo s_colors[PK_RHI_MAX_RENDER_TARGETS]{};
        static VkRenderingAttachmentInfo s_depthStencil;
        s_depthStencil = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };

        const auto& state = m_renderPass[0];

        VkRenderingInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO_KHR };
        renderPassInfo.flags = state.flags;
        renderPassInfo.renderArea = state.area;
        renderPassInfo.layerCount = state.layers;
        renderPassInfo.viewMask = state.viewMask;
        renderPassInfo.colorAttachmentCount = state.colorCount;
        renderPassInfo.pColorAttachments = s_colors;
        renderPassInfo.pDepthAttachment = &s_depthStencil;
        renderPassInfo.pStencilAttachment = nullptr;

        for (auto i = 0u; i < state.colorCount; ++i)
        {
            auto& dst = s_colors[i];
            auto& src = state.colors[i];
            dst.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            dst.pNext = nullptr;
            dst.imageView = src.target->image.view;
            dst.imageLayout = src.finalLayout;
            dst.resolveMode = src.resolveMode;
            dst.resolveImageView = src.resolve ? src.resolve->image.view : nullptr;
            dst.resolveImageLayout = src.resolve ? src.finalLayout : VK_IMAGE_LAYOUT_UNDEFINED;
            dst.loadOp = VulkanEnumConvert::GetLoadOp(src.initialLayout, src.loadOp);
            dst.storeOp = VulkanEnumConvert::GetStoreOp(src.storeOp);
            dst.clearValue = VulkanEnumConvert::GetClearValue(src.clearValue);
        }

        if (state.depth.target)
        {
            auto& src = state.depth;
            s_depthStencil.imageView = src.target->image.view;
            s_depthStencil.imageLayout = src.finalLayout;
            s_depthStencil.resolveMode = src.resolveMode;
            s_depthStencil.resolveImageView = src.resolve ? src.resolve->image.view : nullptr;
            s_depthStencil.resolveImageLayout = src.resolve ? src.finalLayout : VK_IMAGE_LAYOUT_UNDEFINED;
            s_depthStencil.loadOp = VulkanEnumConvert::GetLoadOp(src.initialLayout, src.loadOp);
            s_depthStencil.storeOp = VulkanEnumConvert::IsDepthStencilWrite(src.finalLayout) ? VulkanEnumConvert::GetStoreOp(src.storeOp) : VK_ATTACHMENT_STORE_OP_NONE;
            s_depthStencil.clearValue = VulkanEnumConvert::GetClearValue(src.clearValue);

            if (VulkanEnumConvert::IsDepthStencilFormat(src.target->image.format))
            {
                renderPassInfo.pStencilAttachment = &s_depthStencil;
            }
        }

        return renderPassInfo;
    }

    VulkanVertexBufferBundle VulkanRenderState::GetVertexBufferBundle() const
    {
        VulkanVertexBufferBundle bundle{};

        auto i = 0u;

        for (; i < PK_RHI_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            auto bufferAttrib = &m_pipelineKey.vertexBuffers[i];
            auto handle = m_vertexBuffers[i];

            if (bufferAttrib->stride == 0 || handle == nullptr)
            {
                break;
            }

            bundle.buffers[i] = handle->buffer.buffer;
        }

        bundle.count = i;

        return bundle;
    }

    VulkanDescriptorSetBundle VulkanRenderState::GetDescriptorSetBundle(const FenceRef& fence, uint32_t dirtyFlags)
    {
        VulkanDescriptorSetBundle bundle{};

        if (m_pipelineKey.shader != nullptr)
        {
            bundle.bindPoint = VulkanEnumConvert::GetPipelineBindPoint(m_pipelineKey.shader->GetStageFlags());
            bundle.layout = m_pipelineKey.shader->GetPipelineLayout()->layout;
            bundle.firstSet = 0xFFFFu;
            bundle.count = 0u;

            for (auto i = 0u; i < PK_RHI_MAX_DESCRIPTOR_SETS; ++i)
            {
                if ((dirtyFlags & (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i)) != 0)
                {
                    if (i < bundle.firstSet)
                    {
                        bundle.firstSet = i;
                    }

                    auto set = m_descriptorSets[i];
                    set->fence = fence;
                    bundle.sets[bundle.count++] = set->set;
                }
            }
        }

        return bundle;
    }

    VkStridedDeviceAddressRegionKHR* VulkanRenderState::GetShaderBindingTableAddresses()
    {
        static VkStridedDeviceAddressRegionKHR addresses[(uint32_t)RayTracingShaderGroup::MaxCount];

        if (m_pipelineKey.shader != nullptr)
        {
            for (auto i = 0u; i < (uint32_t)RayTracingShaderGroup::MaxCount; ++i)
            {
                if (m_pipelineKey.shader->HasRayTracingShaderGroup((RayTracingShaderGroup)i))
                {
                    addresses[i] = m_sbtAddresses[i];
                }
            }
        }

        return addresses;
    }

    const VulkanBindHandle* VulkanRenderState::GetIndexBuffer(VkIndexType* outIndexType) const
    {
        *outIndexType = m_indexType;
        return m_indexBuffer;
    }


    void VulkanRenderState::Reset()
    {
        memset(m_descriptorSetKeys, 0, sizeof(m_descriptorSetKeys));
        memset(&m_pipelineKey, 0, sizeof(m_pipelineKey));
        memset(m_renderPass, 0, sizeof(m_renderPass));
        memset(m_viewports, 0, sizeof(m_viewports));
        memset(m_scissors, 0, sizeof(m_scissors));
        memset(m_vertexBuffers, 0, sizeof(m_vertexBuffers));
        memset(m_descriptorSets, 0, sizeof(m_descriptorSets));
        memset(m_vertexStreamLayout, 0, sizeof(m_vertexStreamLayout));

        m_indexType = VK_INDEX_TYPE_UINT16;
        m_pipelineKey.fixedFunctionState = VulkanPipelineCache::FixedFunctionState();
        m_pipeline = nullptr;
        m_indexBuffer = nullptr;
        m_dirtyFlags = PK_RENDER_STATE_DIRTY_RENDERTARGET |
            PK_RENDER_STATE_DIRTY_PIPELINE |
            PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;

        m_renderPass[0].layers = 1;
        m_renderPass[1].layers = 1;
        
        // @TODO where are these defined in dynamic rendering?!?
        //m_renderPassKey[0].samples = VK_SAMPLE_COUNT_1_BIT;
        //m_renderPassKey[1].samples = VK_SAMPLE_COUNT_1_BIT;

        m_pipelineKey.primitiveRestart = VK_FALSE;
    }

    void VulkanRenderState::SetRenderTarget(const VulkanBindHandle* const* renderTargets, const VulkanBindHandle* const* resolves, uint32_t count)
    {
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;

        auto state = &m_renderPass[0];
        *state = {};

        // Assuming same ranges for all render targets.
        state->flags = 0u;
        state->viewMask = 0u;
        state->colorCount = 0u;
        state->dynamicTargets = false;
        state->layers = renderTargets[0]->image.range.layerCount;
        state->area = {{0, 0}, {renderTargets[0]->image.extent.width, renderTargets[0]->image.extent.height}};

        for (auto i = 0u; i < count; ++i)
        {
            auto target = renderTargets[i];
            auto resolve = resolves != nullptr ? resolves[i] : nullptr;
            auto isDepth = VulkanEnumConvert::IsDepthFormat(target->image.format);
            auto attachment = isDepth ? &state->depth : (state->colors + state->colorCount++);

            attachment->target = target;
            attachment->resolve = resolve;
            attachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment->finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment->loadOp = LoadOp::Keep;
            attachment->storeOp = StoreOp::Store;
            attachment->clearValue = {};
            // @TODO support
            attachment->resolveMode = VK_RESOLVE_MODE_NONE;
        }
    }

    void VulkanRenderState::ClearColor(const color& color, uint32_t index)
    {
        auto& attachment = m_renderPass[0].colors[index];
        attachment.loadOp = LoadOp::Clear;
        attachment.clearValue.float32 = color;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
    }

    void VulkanRenderState::ClearDepth(float depth, uint32_t stencil)
    {
        auto& attachment = m_renderPass[0].depth;
        attachment.loadOp = LoadOp::Clear;
        attachment.clearValue.depth = depth;
        attachment.clearValue.stencil = stencil;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
    }

    void VulkanRenderState::DiscardColor(uint32_t index)
    {
        m_renderPass[0].colors[index].loadOp = LoadOp::Discard;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
    }

    void VulkanRenderState::DiscardDepth()
    {
        m_renderPass[0].depth.loadOp = LoadOp::Discard;
        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
    }

    bool VulkanRenderState::SetViewports(const uint4* rects, uint32_t& count, VkViewport** outViewports)
    {
        if (count > PK_RHI_MAX_VIEWPORTS)
        {
            count = PK_RHI_MAX_VIEWPORTS;
        }

        *outViewports = m_viewports;
        bool hasChanged = false;

        for (auto i = 0u; i < count; ++i)
        {
            auto& rect = rects[i];
            VkViewport v = { (float)rect.x, (float)rect.y, (float)rect.z, (float)rect.w, 0.0f, 1.0f };

            if (memcmp(&m_viewports[i], &v, sizeof(VkViewport)) != 0)
            {
                m_viewports[i] = v;
                hasChanged = true;
            }
        }

        return hasChanged;
    }

    bool VulkanRenderState::SetScissors(const uint4* rects, uint32_t& count, VkRect2D** outScissors)
    {
        if (count > PK_RHI_MAX_VIEWPORTS)
        {
            count = PK_RHI_MAX_VIEWPORTS;
        }

        *outScissors = m_scissors;
        bool hasChanged = false;

        for (auto i = 0u; i < count; ++i)
        {
            auto& rect = rects[i];
            VkRect2D v = { {(int)rect.x, (int)rect.y}, { rect.z, rect.w } };

            if (memcmp(&m_scissors[i], &v, sizeof(VkRect2D)) != 0)
            {
                m_scissors[i] = v;
                hasChanged = true;
            }
        }

        return hasChanged;
    }

    void VulkanRenderState::SetShader(const VulkanShader* shader)
    {
        if (m_pipelineKey.shader != shader)
        {
            m_pipelineKey.shader = shader;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE | PK_RENDER_STATE_DIRTY_SHADER;
        }
    }

    void VulkanRenderState::SetStageExcludeMask(const ShaderStageFlags mask)
    {
        if (m_pipelineKey.fixedFunctionState.excludeStageMask != (uint16_t)mask)
        {
            m_pipelineKey.fixedFunctionState.excludeStageMask = (uint16_t)mask;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
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

        for (; i < PK_RHI_MAX_VERTEX_ATTRIBUTES && i < count; ++i)
        {
            auto handle = handles[i];

            PK_THROW_ASSERT(handle != nullptr, "Passing null vertex buffer is not allowed!");

            if (handle != m_vertexBuffers[i])
            {
                m_vertexBuffers[i] = handle;
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;
            }
        }

        if (i < PK_RHI_MAX_VERTEX_ATTRIBUTES)
        {
            memset(m_vertexBuffers + i, 0, sizeof(m_vertexBuffers[0]) * (PK_RHI_MAX_VERTEX_ATTRIBUTES - i));
        }
    }

    void VulkanRenderState::SetVertexStreams(const VertexStreamElement* elements, uint32_t count)
    {
        PK_THROW_ASSERT(count <= PK_RHI_MAX_VERTEX_ATTRIBUTES, "Tried to bind more vertex attributes than currently supported!");

        if (memcmp(m_vertexStreamLayout, elements, sizeof(VertexStreamElement) * count) != 0)
        {
            memcpy(m_vertexStreamLayout, elements, sizeof(VertexStreamElement) * count);
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;
        }

        if (count < PK_RHI_MAX_VERTEX_ATTRIBUTES && m_vertexStreamLayout[count].stride != 0)
        {
            memset(m_vertexStreamLayout + count, 0, sizeof(VertexStreamElement) * (PK_RHI_MAX_VERTEX_ATTRIBUTES - count));
        }
    }

    void VulkanRenderState::SetIndexBuffer(const VulkanBindHandle* handle, VkIndexType indexType)
    {
        if (m_indexBuffer != handle || (handle != nullptr && indexType != m_indexType))
        {
            m_indexBuffer = handle;
            m_indexType = indexType;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_INDEXBUFFER;
        }
    }

    void VulkanRenderState::SetShaderBindingTableAddress(RayTracingShaderGroup group, VkDeviceAddress address, size_t stride, size_t size)
    {
        m_sbtAddresses[(uint32_t)group] = { address, stride, size };
    }


    void VulkanRenderState::RecordBuffer(const VulkanBindHandle* handle, VkPipelineStageFlags stage, VkAccessFlags access)
    {
        if (handle->isTracked)
        {
            auto handler = m_services.barrierHandler;
            VulkanBarrierHandler::AccessRecord record{};
            record.stage = stage;
            record.access = access;
            record.bufferRange.offset = (uint32_t)handle->buffer.offset;
            record.bufferRange.size = (uint32_t)handle->buffer.range;
            record.queueFamily = handle->isConcurrent ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : handler->GetQueueFamily();
            handler->Record(handle->buffer.buffer, record, PK_RHI_ACCESS_OPT_BARRIER);
        }
    }

    void VulkanRenderState::RecordImage(const VulkanBindHandle* handle, VkPipelineStageFlags stage, VkAccessFlags access, VkImageLayout overrideLayout, uint8_t options)
    {
        if (handle->isTracked)
        {
            auto handler = m_services.barrierHandler;
            VulkanBarrierHandler::AccessRecord record{};
            record.stage = stage;
            record.access = access;
            record.imageRange = VulkanConvertRange(handle->image.range);
            record.aspect = handle->image.range.aspectMask;
            record.layout = overrideLayout != VK_IMAGE_LAYOUT_MAX_ENUM ? overrideLayout : handle->image.layout;
            record.queueFamily = handle->isConcurrent ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : handler->GetQueueFamily();
            handler->Record(handle->image.image, record, options);

            // Track alias as well so that layout stays consistent
            if (handle->image.alias)
            {
                handler->Record(handle->image.alias, record, options);
            }
        }
    }

    VulkanBarrierHandler::AccessRecord VulkanRenderState::RecordRenderTarget(const VulkanBindHandle* handle, 
        VkPipelineStageFlags stage,
        VkAccessFlags access,
        VkImageLayout layout,
        uint8_t options)
    {
        auto handler = m_services.barrierHandler;

        VulkanBarrierHandler::AccessRecord record{};
        record.stage = stage;
        record.access = access;
        record.imageRange = VulkanConvertRange(handle->image.range);
        record.aspect = handle->image.range.aspectMask;
        record.layout = layout;
        record.queueFamily = handle->isConcurrent ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : handler->GetQueueFamily();

        if (handle->isTracked)
        {
            auto previous = handler->Retrieve(handle->image.image, record);
            handler->Record(handle->image.image, record, options);
            return previous;
        }

        return record;
    }


    void VulkanRenderState::ValidateRenderTarget()
    {
        auto shader = m_pipelineKey.shader;
        auto stageFlags = shader->GetStageFlags();

        if ((stageFlags & ShaderStageFlags::StagesGraphics) == 0u || (m_dirtyFlags & PK_RENDER_STATE_DIRTY_RENDERTARGET) == 0)
        {
            return;
        }

        if (memcmp(m_renderPass, m_renderPass + 1, sizeof(VulkanRenderPassState)) == 0)
        {
            m_dirtyFlags &= ~PK_RENDER_STATE_DIRTY_RENDERTARGET;
            return;
        }

        memcpy(m_renderPass + 1, m_renderPass, sizeof(VulkanRenderPassState));

        {
            auto& formats = m_pipelineKey.fixedFunctionState.colorFormats;
            auto& colors = m_renderPass[0].colors;
            auto& depth = m_renderPass[0].depth;

            for (auto i = 0u; i < PK_RHI_MAX_RENDER_TARGETS; ++i)
            {
                formats[i] = i < m_renderPass->colorCount ? colors[i].target->image.format : VK_FORMAT_UNDEFINED;
            }

            m_pipelineKey.fixedFunctionState.depthFormat = depth.target ? depth.target->image.format : VK_FORMAT_UNDEFINED;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
        }

        {
            auto& colors = m_renderPass[0].colors;
            auto& depth = m_renderPass[0].depth;

            for (auto i = 0u; i < m_renderPass[0].colorCount; ++i)
            {
                auto& color = colors[i];

                if (color.target && color.target->image.image)
                {
                    // Invalidate image
                    if (color.loadOp != LoadOp::Keep)
                    {
                        RecordImage(color.target, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0u, VK_IMAGE_LAYOUT_UNDEFINED, 0u);
                    }

                    auto previous = RecordRenderTarget(color.target,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        PK_RHI_ACCESS_OPT_BARRIER);

                    color.initialLayout = previous.layout;
                    color.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    
                    if (color.resolve && color.resolve->image.image)
                    {
                        previous = RecordRenderTarget(color.target,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            PK_RHI_ACCESS_OPT_BARRIER);
                    }
                }
            }

            if (depth.target && depth.target->image.image)
            {
                const auto isStencil = VulkanEnumConvert::IsDepthStencilFormat(depth.target->image.format);
                const auto isReadOnly = !m_pipelineKey.fixedFunctionState.depthStencil.depthWriteEnable &&
                                        (!isStencil || m_pipelineKey.fixedFunctionState.depthStencil.stencilTestEnable);

                const VkImageLayout layouts[2][2] =
                {
                    { VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL },
                    { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL }
                };

                const auto layout = layouts[isStencil][isReadOnly];

                auto previous = RecordRenderTarget(depth.target,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    layout,
                    PK_RHI_ACCESS_OPT_BARRIER);

                depth.initialLayout = previous.layout;
                depth.finalLayout = layout;
            }
        }
    }

    void VulkanRenderState::ValidateVertexBuffers()
    {
        auto shader = m_pipelineKey.shader;
        auto stageFlags = shader->GetStageFlags();

        if ((ShaderStageFlags::StagesVertex & stageFlags) == 0u)
        {
            return;
        }

        auto index = 0u;
        auto validateAttributes = false;
        const auto& shaderLayout = shader->GetVertexLayout();

        if (m_dirtyFlags & PK_RENDER_STATE_DIRTY_SHADER)
        {
            for (const auto& element : shaderLayout)
            {
                auto* attribute = m_pipelineKey.vertexAttributes + index++;
                auto format = VulkanEnumConvert::GetFormat(element->format);

                if (attribute->location != element->location || attribute->format != format)
                {
                    validateAttributes = true;
                    attribute->location = element->location;
                    attribute->format = format;
                }
            }

            // Attribute count changed
            if (index < PK_RHI_MAX_VERTEX_ATTRIBUTES && m_pipelineKey.vertexAttributes[index].format != VK_FORMAT_UNDEFINED)
            {
                memset(m_pipelineKey.vertexAttributes + index, 0, sizeof(VkVertexInputAttributeDescription) * (PK_RHI_MAX_VERTEX_ATTRIBUTES - index));
                validateAttributes = true;
            }
        }

        if (!validateAttributes && (m_dirtyFlags & PK_RENDER_STATE_DIRTY_VERTEXBUFFERS) == 0)
        {
            return;
        }

        auto indexBindingBuffer = 0u;
        auto boundElementCount = 0u;
        const VertexStreamElement* elementPrev = nullptr;

        for (index = 0u; index < PK_RHI_MAX_VERTEX_ATTRIBUTES && m_vertexStreamLayout[index].stride != 0; ++index)
        {
            const auto& element = m_vertexStreamLayout[index];
            auto elementIdx = 0u;

            if (m_vertexBuffers[element.stream] && shaderLayout.TryGetElement(element.name, &elementIdx))
            {
                // Merge adjacent buffer bindings if possible
                if (elementPrev && (elementPrev->stream != element.stream ||
                    elementPrev->inputRate != element.inputRate ||
                    elementPrev->stride != element.stride))
                {
                    ++indexBindingBuffer;
                }

                ++boundElementCount;
                elementPrev = &element;
                auto inputRate = VulkanEnumConvert::GetInputRate(element.inputRate);
                auto bindingBuffer = &m_pipelineKey.vertexBuffers[indexBindingBuffer];
                auto bindingAttribute = &m_pipelineKey.vertexAttributes[elementIdx];

                if (bindingBuffer->binding != element.stream || bindingBuffer->inputRate != inputRate || bindingBuffer->stride != element.stride)
                {
                    m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
                    bindingBuffer->binding = element.stream;
                    bindingBuffer->inputRate = inputRate;
                    bindingBuffer->stride = element.stride;
                }

                if (bindingAttribute->binding != indexBindingBuffer || bindingAttribute->offset != element.offset)
                {
                    m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
                    bindingAttribute->binding = indexBindingBuffer;
                    bindingAttribute->offset = element.offset;
                }
            }
        }

        if (boundElementCount != shaderLayout.GetCount())
        {
            PK_LOG_WARNING("Warning only '%u' out of '%u' shader vertex input streams were bound.", boundElementCount, shaderLayout.GetCount());
        }

        auto bindingBufferCount = indexBindingBuffer + 1u;

        if (bindingBufferCount < PK_RHI_MAX_VERTEX_ATTRIBUTES && m_pipelineKey.vertexBuffers[bindingBufferCount].stride != 0)
        {
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
            memset(m_pipelineKey.vertexBuffers + bindingBufferCount, 0, sizeof(VkVertexInputBindingDescription) * (PK_RHI_MAX_VERTEX_ATTRIBUTES - bindingBufferCount));
        }
    }

    void VulkanRenderState::ValidateDescriptorSets(const FenceRef& fence)
    {
        auto resources = m_services.globalResources;
        auto shader = m_pipelineKey.shader;
        auto setCount = shader->GetDescriptorSetCount();
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

                if (element->count > 1)
                {
                    PK_THROW_ASSERT(resources->TryGet(element->name, wrappedHandleArray), "Descriptors (%s) not bound!", element->name.c_str());

                    uint32_t version = 0u;
                    uint32_t count = 0u;
                    auto handles = wrappedHandleArray.handle->GetHandles(&version, &count);
                    count = count < element->count ? (uint16_t)count : element->count;

                    if (binding->count != count || binding->type != element->type || binding->handles != handles || binding->version != version || !binding->isArray)
                    {
                        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i;
                        binding->count = count;
                        binding->type = element->type;
                        binding->handles = handles;
                        binding->version = version;
                        binding->isArray = true;
                    }

                    continue;
                }

                PK_THROW_ASSERT(resources->TryGet(element->name, wrappedHandle), "Descriptor (%s) not bound!", element->name.c_str());
                auto handle = wrappedHandle.handle;

                if (binding->count != element->count || binding->type != element->type || binding->handle != handle || binding->version != handle->Version() || binding->isArray)
                {
                    m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i;
                    binding->count = element->count;
                    binding->type = element->type;
                    binding->handle = handle;
                    binding->version = handle->Version();
                    binding->isArray = false;
                }
            }

            // Binding count changed
            if (index < PK_RHI_MAX_DESCRIPTORS_PER_SET && bindings[index].count != 0)
            {
                memset(bindings + index, 0, sizeof(bindings[0]) * (PK_RHI_MAX_DESCRIPTORS_PER_SET - index));
                m_dirtyFlags |= (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i);
            }

            if (m_dirtyFlags & (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i))
            {
                auto name = shader->GetName();
                m_descriptorSets[i] = m_services.descriptorCache->GetDescriptorSet(shader->GetDescriptorSetLayout(i), m_descriptorSetKeys[i], fence, name);
            }
        }

        // If a lower number set has changed all sets above it need to be rebound.
        //for (auto i = 0; i < (int32_t)(setCount - 1); ++i)
        //{
        //    if ((m_dirtyFlags & (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i)) != 0)
        //    {
        //        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << (i + 1);
        //    }
        //}

        // @TODO investigate why this happens.
        // Unfortunately for some reason a set is being unbound despite using the same layout & resources (in the same pipeline bind point).
        // I'll just have to dirty all of them :/
        if ((m_dirtyFlags & PK_RENDER_STATE_DIRTY_DESCRIPTOR_SETS) != 0)
        {
            for (auto i = 0u; i < setCount; ++i)
            {
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i;
            }
        }

        // Clear remaining keys as they will go unbound when this pipe is used
        if (setCount < PK_RHI_MAX_DESCRIPTOR_SETS)
        {
            memset(m_descriptorSetKeys + setCount, 0, sizeof(m_descriptorSetKeys[0]) * (PK_RHI_MAX_DESCRIPTOR_SETS - setCount));
        }
    }

    PKRenderStateDirtyFlags VulkanRenderState::ValidatePipeline(const FenceRef& fence)
    {
        PK_THROW_ASSERT(m_pipelineKey.shader != nullptr, "Pipeline validation failed! Shader is unassigned!");

        ValidateRenderTarget();
        ValidateVertexBuffers();
        ValidateDescriptorSets(fence);
        RecordResourceAccess();

        if (m_dirtyFlags & PK_RENDER_STATE_DIRTY_PIPELINE)
        {
            m_pipeline = m_services.pipelineCache->GetPipeline(m_pipelineKey);
        }

        auto stageFlags = m_pipelineKey.shader->GetStageFlags();
        auto vertexFlags = m_dirtyFlags & (PK_RENDER_STATE_DIRTY_VERTEXBUFFERS | PK_RENDER_STATE_DIRTY_INDEXBUFFER);
        auto graphicsFlags = m_dirtyFlags & PK_RENDER_STATE_DIRTY_RENDERTARGET;
        auto flags = m_dirtyFlags;
        m_dirtyFlags = 0u;

        // Dont dirty pipeline type specific flags when not required.
        // Restore dirty state for future resolve.
        if ((ShaderStageFlags::StagesVertex & stageFlags) == 0u)
        {
            flags &= ~vertexFlags;
            m_dirtyFlags |= vertexFlags;

        }

        if ((ShaderStageFlags::StagesGraphics & stageFlags) == 0)
        {
            flags &= ~graphicsFlags;
            m_dirtyFlags |= graphicsFlags;
        }

        return (PKRenderStateDirtyFlags)flags;
    }


    void VulkanRenderState::RecordResourceAccess()
    {
        auto shader = m_pipelineKey.shader;
        auto stageFlags = shader->GetStageFlags();
        auto setCount = m_pipelineKey.shader->GetDescriptorSetCount();

        for (auto i = 0u; i < setCount; ++i)
        {
            const auto& setkey = m_descriptorSetKeys[i];

            for (auto j = 0u; j < PK_RHI_MAX_DESCRIPTORS_PER_SET && setkey.bindings[j].count > 0; ++j)
            {
                auto binding = setkey.bindings[j];

                // No Array support as they're locally reserved for readonly resources
                if (binding.isArray)
                {
                    continue;
                }

                auto handle = binding.handle;
                auto stage = VulkanEnumConvert::GetPipelineStageFlags(setkey.stageFlags);
                auto access = shader->GetResourceLayout(i)[j].writeStageMask != 0u ? VK_ACCESS_SHADER_WRITE_BIT : VK_ACCESS_NONE;

                switch (binding.type)
                {
                    case ShaderResourceType::SamplerTexture:
                    case ShaderResourceType::Texture:
                    case ShaderResourceType::Image:
                        RecordImage(handle, stage, access | VK_ACCESS_SHADER_READ_BIT);
                        break;
                    case ShaderResourceType::StorageBuffer:
                    case ShaderResourceType::DynamicStorageBuffer:
                        RecordBuffer(handle, stage, access | VK_ACCESS_SHADER_READ_BIT);
                        break;
                    case ShaderResourceType::ConstantBuffer:
                    case ShaderResourceType::DynamicConstantBuffer:
                        RecordBuffer(handle, stage, access | VK_ACCESS_UNIFORM_READ_BIT);
                        break;
                    default: break;
                }
            }
        }

        if ((ShaderStageFlags::StagesVertex & stageFlags) != 0u)
        {
            for (auto i = 0u; i < PK_RHI_MAX_VERTEX_ATTRIBUTES && m_vertexBuffers[i]; ++i)
            {
                RecordBuffer(m_vertexBuffers[i], VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
            }

            if (m_indexBuffer != nullptr)
            {
                RecordBuffer(m_indexBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT);
            }
        }
    }

}