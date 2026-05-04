#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanBindSet.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanRenderState.h"

namespace PK
{
    VkRenderingInfo VulkanRenderState::GetRenderPassInfo() const
    {
        static VkRenderingAttachmentInfo s_colors[PK_RHI_MAX_RENDER_TARGETS]{};
        static VkRenderingAttachmentInfo s_depthStencil;
        s_depthStencil = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };

        VkRenderingInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO_KHR };
        renderPassInfo.flags = 0u;
        renderPassInfo.renderArea = m_renderTarget.area;
        renderPassInfo.layerCount = m_renderTarget.layers;
        renderPassInfo.viewMask = 0u;
        renderPassInfo.colorAttachmentCount = m_renderTarget.colorCount;
        renderPassInfo.pColorAttachments = s_colors;
        renderPassInfo.pDepthAttachment = &s_depthStencil;
        renderPassInfo.pStencilAttachment = nullptr;

        for (auto i = 0u; i < m_renderTarget.colorCount; ++i)
        {
            auto& dst = s_colors[i];
            auto& src = m_renderTarget.colors[i];
            dst.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            dst.pNext = nullptr;
            dst.imageView = src.target->image.view;
            dst.imageLayout = m_renderTargetLayouts[i];
            dst.resolveMode = src.resolveMode;
            dst.resolveImageView = src.resolve ? src.resolve->image.view : nullptr;
            dst.resolveImageLayout = src.resolve ? dst.imageLayout : VK_IMAGE_LAYOUT_UNDEFINED;
            dst.loadOp = VulkanEnumConvert::GetLoadOp(src.loadOp);
            dst.storeOp = VulkanEnumConvert::GetStoreOp(src.storeOp);
            dst.clearValue = VulkanEnumConvert::GetClearValue(src.clearValue);
        }

        if (m_renderTarget.depth.target)
        {
            auto& src = m_renderTarget.depth;
            s_depthStencil.imageView = src.target->image.view;
            s_depthStencil.imageLayout = m_renderTargetLayouts[PK_RHI_MAX_RENDER_TARGETS];
            s_depthStencil.resolveMode = src.resolveMode;
            s_depthStencil.resolveImageView = src.resolve ? src.resolve->image.view : nullptr;
            s_depthStencil.resolveImageLayout = src.resolve ? s_depthStencil.imageLayout : VK_IMAGE_LAYOUT_UNDEFINED;
            s_depthStencil.loadOp = VulkanEnumConvert::GetLoadOp(src.loadOp);
            s_depthStencil.storeOp = VulkanEnumConvert::GetStoreOp(src.storeOp);
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

        for (auto& stream : m_pipelineKey.vertexStreams)
        {
            if (stream.stride != 0)
            {
                bundle.buffers[stream.binding] = m_vertexBuffers[stream.binding]->buffer.buffer;
                bundle.count = math::max(bundle.count, stream.binding + 1u);
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
        memset(&m_pipelineKey, 0, sizeof(m_pipelineKey));
        memset(&m_renderTarget, 0, sizeof(m_renderTarget));
        memset(&m_descritorState, 0, sizeof(m_descritorState));
        memset(m_viewports, 0, sizeof(m_viewports));
        memset(m_scissors, 0, sizeof(m_scissors));
        memset(m_vertexBuffers, 0, sizeof(m_vertexBuffers));
        memset(m_vertexStreamLayout, 0, sizeof(m_vertexStreamLayout));

        m_indexType = VK_INDEX_TYPE_UINT16;
        m_pipelineKey.fixedFunctionState = VulkanPipelineCache::FixedFunctionState();
        m_pipeline = nullptr;
        m_indexBuffer = nullptr;
        m_dirtyFlags = PK_RENDER_STATE_DIRTY_RENDERTARGET |
            PK_RENDER_STATE_DIRTY_PIPELINE |
            PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;

        m_renderTarget.layers = 1;
    }

    bool VulkanRenderState::SetViewports(const uint4* rects, uint32_t& count, VkViewport** outViewports)
    {
        *outViewports = m_viewports;
        bool hasChanged = false;

        for (auto i = 0u; i < count && i < PK_RHI_MAX_VIEWPORTS; ++i)
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
        *outScissors = m_scissors;

        if (memcmp(m_scissors, rects, sizeof(VkRect2D) * (count > PK_RHI_MAX_VIEWPORTS ? PK_RHI_MAX_VIEWPORTS : count)) != 0)
        {
            memcpy(m_scissors, rects, sizeof(VkRect2D) * (count > PK_RHI_MAX_VIEWPORTS ? PK_RHI_MAX_VIEWPORTS : count));
            return true;
        }

        return false;
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
    
    
    void VulkanRenderState::SetRenderTarget(const VulkanRenderTargetBindings& target)
    {
        if (memcmp(&m_renderTarget, &target, sizeof(VulkanRenderTargetBindings)) != 0)
        {
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_RENDERTARGET;
            m_renderTarget = target;
        }
    }

    void VulkanRenderState::SetShader(const VulkanShader* shader)
    {
        if (m_pipelineKey.shader != shader)
        {
            m_pipelineKey.shader = shader;
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE | PK_RENDER_STATE_DIRTY_SHADER;
        }
    }

    void VulkanRenderState::SetVertexBuffers(const VulkanBindHandle** handles, uint32_t count)
    {
        auto i = 0u;

        for (; i < count && i < PK_RHI_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            auto handle = handles[i];

            PK_DEBUG_FATAL_ASSERT(handle != nullptr, "Passing null vertex buffer is not allowed!");

            if (handle != m_vertexBuffers[i])
            {
                m_vertexBuffers[i] = handle;
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;
            }
        }

        if (i < PK_RHI_MAX_VERTEX_ATTRIBUTES)
        {
            Memory::Memset(m_vertexBuffers + i, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - i);
        }
    }

    void VulkanRenderState::SetVertexStreams(const VertexStreamElement* elements, uint32_t count)
    {
        PK_DEBUG_FATAL_ASSERT(count <= PK_RHI_MAX_VERTEX_ATTRIBUTES, "Tried to bind more vertex attributes than currently supported!");

        if (memcmp(m_vertexStreamLayout, elements, sizeof(VertexStreamElement) * count) != 0)
        {
            memcpy(m_vertexStreamLayout, elements, sizeof(VertexStreamElement) * count);
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_VERTEXBUFFERS;
        }

        if (count < PK_RHI_MAX_VERTEX_ATTRIBUTES && m_vertexStreamLayout[count].stride != 0)
        {
            Memory::Memset(m_vertexStreamLayout + count, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - count);
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

    VkImageLayout VulkanRenderState::RecordRenderTarget(const VulkanBindHandle* handle,
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
            return previous.layout;
        }

        return record.layout;
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
        auto validateStreams = false;
        const auto& shaderLayout = shader->GetVertexLayout();

        if (m_dirtyFlags & PK_RENDER_STATE_DIRTY_SHADER)
        {
            for (const auto& element : shaderLayout)
            {
                auto* attribute = m_pipelineKey.vertexAttributes + index++;
                auto format = VulkanEnumConvert::GetFormat(element.format);

                if (attribute->location != element.location || attribute->format != format)
                {
                    validateStreams = true;
                    attribute->location = element.location;
                    attribute->format = format;
                }
            }

            // Attribute count changed
            if (index < PK_RHI_MAX_VERTEX_ATTRIBUTES && m_pipelineKey.vertexAttributes[index].format != VK_FORMAT_UNDEFINED)
            {
                validateStreams = true;
                Memory::Memset(m_pipelineKey.vertexAttributes + index, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - index);
            }
        }

        if (validateStreams || (m_dirtyFlags & PK_RENDER_STATE_DIRTY_VERTEXBUFFERS))
        {
            auto streamIndex = 0u;
            auto remainingAttributes = shaderLayout.GetCount();
            const VertexStreamElement* elementPrev = nullptr;

            for (index = 0u; index < PK_RHI_MAX_VERTEX_ATTRIBUTES && m_vertexStreamLayout[index].stride != 0; ++index)
            {
                const auto& element = m_vertexStreamLayout[index];
                auto elementIdx = 0u;

                if (m_vertexBuffers[element.stream] && shaderLayout.TryGetElement(element.name, &elementIdx))
                {
                    // Merge adjacent buffer bindings if possible
                    // If the user has bound the same stream multiple times with different attributes
                    // we cannot directly use the stream index to actually index the bindings.
                    if (elementPrev && (elementPrev->stream != element.stream ||
                        elementPrev->inputRate != element.inputRate ||
                        elementPrev->stride != element.stride))
                    {
                        ++streamIndex;
                    }

                    --remainingAttributes;
                    elementPrev = &element;
                    auto inputRate = VulkanEnumConvert::GetInputRate(element.inputRate);
                    auto stream = &m_pipelineKey.vertexStreams[streamIndex];
                    auto attribute = &m_pipelineKey.vertexAttributes[elementIdx];

                    if (stream->binding != element.stream || stream->inputRate != inputRate || stream->stride != element.stride)
                    {
                        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
                        stream->binding = element.stream;
                        stream->inputRate = inputRate;
                        stream->stride = element.stride;
                    }

                    if (attribute->binding != streamIndex || attribute->offset != element.offset)
                    {
                        m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
                        attribute->binding = streamIndex;
                        attribute->offset = element.offset;
                    }
                }
            }

            PK_DEBUG_WARNING_ASSERT(remainingAttributes == 0, "Warning '%u' of shader vertex input attributes were not bound.", remainingAttributes);

            auto streamCount = streamIndex + 1u;

            if (streamCount < PK_RHI_MAX_VERTEX_ATTRIBUTES && m_pipelineKey.vertexStreams[streamCount].stride != 0)
            {
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
                Memory::Memset(m_pipelineKey.vertexStreams + streamCount, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - streamCount);
            }
        }
    }

    void VulkanRenderState::ValidateDescriptors(const FenceRef& fence)
    {
        auto resources = m_services.globalResources;
        auto shader = m_pipelineKey.shader;
        auto bindPoint = VulkanEnumConvert::GetPipelineBindPoint(m_pipelineKey.shader->GetStageFlags());
        auto* layout = shader->GetDescriptorSetLayout();
        auto& resourceLayout = shader->GetResourceLayout();
        auto* bindings = m_descritorState.bindings;
        auto bindingIndex = 0u;

        for (const auto& element : resourceLayout)
        {
            auto& binding = bindings[bindingIndex++];
            const auto isVariableSize = element.count == PK_RHI_MAX_UNBOUNDED_SIZE;
            const VulkanBindHandle* const* handles = nullptr;
            uint32_t version = 0u, count = 0u;

            if (isVariableSize)
            {
                const VulkanBindSet* handleSet = nullptr;
                PK_FATAL_ASSERT(resources->TryGet<const VulkanBindSet*>(element.name, handleSet), "Descriptors '%s' not bound!", element.name.c_str());
                handles = handleSet->GetHandles(&version, &count);
            }
            else
            {
                auto size = 0ull;
                PK_FATAL_ASSERT(resources->TryGet<const VulkanBindHandle*>(element.name, &handles, &size), "Descriptor '%s' not bound!", element.name.c_str());
                PK_FATAL_ASSERT(size == sizeof(void*) * element.count, "Descriptor '%s' bound array size '%u' doesn't match size '%u' in shader", element.name.c_str(), size / sizeof(void*), element.count);
                version = handles[0]->Version();
                count = element.count;
            }

            // Hash fixed size array version.
            for (auto i = 1u; i < count && !isVariableSize; ++i)
            {
                auto hash = (uint32_t)handles[i]->Version();
                hash += 0x9e3779b9u + (version << 6u) + (version >> 2u);
                version ^= hash;
            }

            if (binding.handles != handles || binding.count != count || binding.type != element.type || binding.version != version)
            {
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTORS;
                binding.handles = handles;
                binding.count = count;
                binding.type = element.type;
                binding.version = version;
                binding.isVariableSize = isVariableSize;
            }
        }

        // For memory:
        // For some reason this we never encountered a scenario where the first descriptors were identical but only the count was different.
        // Caught with debug engine draw. :)
        if (m_descritorState.bindingCount != resourceLayout.GetCount())
        {
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTORS;
            m_descritorState.bindingCount = (uint32_t)resourceLayout.GetCount();
        }

        if (m_dirtyFlags & PK_RENDER_STATE_DIRTY_DESCRIPTORS)
        {
            auto name = shader->GetName();
            m_descritorState.descriptorSet = m_services.descriptorCache->GetDescriptorSet(layout, bindings, bindingIndex, fence, name);
        }

        // @TODO Technically we should maintain different sets for different bind points but...
        if (m_descritorState.bindPoint != bindPoint)
        {
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTORS;
            m_descritorState.bindPoint = bindPoint;
        }

        if (m_descritorState.stageFlags != layout->stageFlags)
        {
            m_dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTORS;
            m_descritorState.stageFlags = layout->stageFlags;
        }

        m_services.descriptorCache->SetDescriptorSetFence(m_descritorState.descriptorSet, fence);
    }

    void VulkanRenderState::ValidateResourceAccess()
    {
        auto shader = m_pipelineKey.shader;
        auto stageFlags = shader->GetStageFlags();

        for (auto bindingIndex = 0u; bindingIndex < m_descritorState.bindingCount; ++bindingIndex)
        {
            const auto& binding = m_descritorState.bindings[bindingIndex];
            auto access = shader->GetResourceLayout()[bindingIndex].writeMask != 0u ? VK_ACCESS_SHADER_WRITE_BIT : VK_ACCESS_NONE;
            auto stage = VulkanEnumConvert::GetPipelineStageFlags(m_descritorState.stageFlags);

            // No dynamic array support as they're locally reserved for readonly resources
            for (auto i = 0u; i < binding.count && !binding.isVariableSize; ++i)
            {
                auto handle = binding.handles[i];

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

        // Note this might be invalid if the target was accessed between draws within the same render pass.
        // Currently nothing does, Until we get that assert, lets do nothing and save some time.
        if ((ShaderStageFlags::StagesGraphics & stageFlags) != 0u && (m_dirtyFlags & PK_RENDER_STATE_DIRTY_RENDERTARGET) != 0)
        {
            auto& colors = m_renderTarget.colors;
            auto& depth = m_renderTarget.depth;

            for (auto i = 0u; i < m_renderTarget.colorCount; ++i)
            {
                auto& color = colors[i];

                if (color.target && color.target->image.image)
                {
                    // Invalidate image
                    if (color.loadOp != LoadOp::Load)
                    {
                        RecordImage(color.target, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u, VK_IMAGE_LAYOUT_UNDEFINED, 0u);
                    }
                    
                    m_renderTargetLayouts[i] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                    auto previousLayout = RecordRenderTarget(
                        color.target,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        m_renderTargetLayouts[i],
                        PK_RHI_ACCESS_OPT_BARRIER);

                    if (color.loadOp == LoadOp::Load && previousLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                    {
                        color.loadOp = LoadOp::Discard;
                    }

                    if (color.resolve && color.resolve->image.image)
                    {
                        RecordImage(
                            color.target, 
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
                            m_renderTargetLayouts[i], 
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

                m_renderTargetLayouts[PK_RHI_MAX_RENDER_TARGETS] = layouts[isStencil][isReadOnly];

                auto accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | (!isReadOnly ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0u);
                
                // Invalidate image
                if (depth.loadOp != LoadOp::Load)
                {
                    RecordImage(depth.target, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0u, VK_IMAGE_LAYOUT_UNDEFINED, 0u);
                }

                auto previousLayout = RecordRenderTarget(
                    depth.target,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    accessFlags,
                    m_renderTargetLayouts[PK_RHI_MAX_RENDER_TARGETS],
                    PK_RHI_ACCESS_OPT_BARRIER);

                if (depth.loadOp == LoadOp::Load && previousLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                {
                    depth.loadOp = LoadOp::Discard;
                }

                if (isReadOnly)
                {
                    depth.storeOp = StoreOp::None;
                }
            }
        }
    }

    void VulkanRenderState::ValidatePipelineFormats()
    {
        auto shader = m_pipelineKey.shader;
        auto stageFlags = shader->GetStageFlags();

        // Validate pipeline target formats
        if ((stageFlags & ShaderStageFlags::StagesGraphics) != 0u && (m_dirtyFlags & PK_RENDER_STATE_DIRTY_RENDERTARGET) != 0)
        {
            auto& fixed = m_pipelineKey.fixedFunctionState;
            auto& colors = m_renderTarget.colors;
            auto& depth = m_renderTarget.depth;

            for (auto i = 0u; i < PK_RHI_MAX_RENDER_TARGETS; ++i)
            {
                auto format = i < m_renderTarget.colorCount ? colors[i].target->image.format : VK_FORMAT_UNDEFINED;

                if (format != fixed.colorFormats[i])
                {
                    fixed.colorFormats[i] = format;
                    m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
                }
            }

            auto depthFormat = depth.target ? depth.target->image.format : VK_FORMAT_UNDEFINED;

            if (depthFormat != fixed.depthFormat)
            {
                fixed.depthFormat = depthFormat;
                m_dirtyFlags |= PK_RENDER_STATE_DIRTY_PIPELINE;
            }
        }
    }

    PKRenderStateDirtyFlags VulkanRenderState::ValidatePipeline(const FenceRef& fence)
    {
        PK_DEBUG_FATAL_ASSERT(m_pipelineKey.shader != nullptr, "Pipeline validation failed! Shader is unassigned!");

        ValidateVertexBuffers();
        ValidateDescriptors(fence);
        ValidateResourceAccess();
        ValidatePipelineFormats();

        if ((m_dirtyFlags & PK_RENDER_STATE_DIRTY_PIPELINE) != 0u)
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
}
