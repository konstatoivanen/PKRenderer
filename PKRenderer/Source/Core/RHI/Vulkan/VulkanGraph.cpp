#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "Core/RHI/Vulkan/VulkanBindSet.h"
#include "Core/RHI/Vulkan/VulkanSwapchain.h"
#include "Core/RHI/Vulkan/VulkanAccelerationStructure.h"
#include "Core/RHI/Vulkan/VulkanAccessRecorder.h"
#include "VulkanGraph.h"

namespace PK
{
    struct VulkanGraphDebugScopes
    {
        VulkanDebugCmd* debugStack[PK_VK_MAX_DEBUG_STACK]{};
        VulkanTimerCmd* timerStack[PK_VK_MAX_TIMER_STACK]{};
        uint32_t timerQueries[(uint32_t)QueueType::EnumCount][PK_VK_MAX_TIMER_STACK]{};
        uint32_t debugCount[(uint32_t)QueueType::EnumCount]{};
        uint32_t debugMin[(uint32_t)QueueType::EnumCount]{};
        uint32_t timerCount[(uint32_t)QueueType::EnumCount]{};
        uint32_t timerMin[(uint32_t)QueueType::EnumCount]{};
        uint32_t debugHead = 0u;
        uint32_t timerHead = 0u;
    };

    struct VulkanGraphContext
    {
        VulkanGraph* graph = nullptr;
        VulkanQueue* queue = nullptr;
        VulkanCommandBuffer* cmb = nullptr;
        VulkanGraphDebugScopes* debugScopes = nullptr;
        uint32_t queueIndex = 0u;
    };

    VulkanGraph::VulkanGraph(const VulkanDriver* driver) : 
        m_globalResources(PK_VK_GLOBAL_PROPERTIES_INITIAL_SIZE, PK_VK_GLOBAL_PROPERTIES_INITIAL_COUNT),
        m_accessRecorder(),
        m_driver(driver),
        m_stagingArena(driver->stagingArena),
        m_descriptorArena(driver->descriptorArena),
        m_timerArena(driver->timerArena),
        m_invocationCounter(0ull)
    {
        BeginGraph();
    }

    VulkanGraph::~VulkanGraph() 
    {
        EndGraph(); 
        m_invocationCounter = ~0ull;
    }

    #define PK_VK_BIND_HANDLES(name, assigner, count)\
    auto handles = PK_STACK_ALLOC(const VulkanBindHandle*, count);\
    for (auto i = 0u; i < (uint32_t)count; ++i) handles[i] = assigner;\
    m_globalResources.Set(name, handles, (uint32_t)count)                   

    void VulkanGraph::SetBuffers(NameID name, RHIBuffer** buffers, const BufferIndexRange* ranges, size_t count) { PK_VK_BIND_HANDLES(name, buffers[i]->GetNativeView<VulkanBindHandle>(ranges[i]), count); }
    void VulkanGraph::SetTextures(NameID name, RHITexture** textures, const TextureViewRange* ranges, size_t count) { PK_VK_BIND_HANDLES(name, textures[i]->GetNativeView<VulkanBindHandle>(ranges[i], TextureViewMode::SRV), count); }
    void VulkanGraph::SetImages(NameID name, RHITexture** images, const TextureViewRange* ranges, size_t count) { PK_VK_BIND_HANDLES(name, images[i]->GetNativeView<VulkanBindHandle>(ranges[i], TextureViewMode::UAV), count); }
    void VulkanGraph::SetSamplers(NameID name, const SamplerDescriptor* samplers, size_t count) { PK_VK_BIND_HANDLES(name, m_driver->samplerCache->GetBindHandle(samplers[i]), count); }
    void VulkanGraph::SetAccelerationStructures(NameID name, RHIAccelerationStructure** structures, size_t count) { PK_VK_BIND_HANDLES(name, static_cast<VulkanAccelerationStructure*>(structures[i])->GetBindHandle(), count); }
    void VulkanGraph::SetBufferSet(NameID name, RHIBufferBindSet* bufferArray) { m_globalResources.Set(name, static_cast<const VulkanBindSet*>(bufferArray)); }
    void VulkanGraph::SetTextureSet(NameID name, RHITextureBindSet* textureArray) { m_globalResources.Set(name, static_cast<const VulkanBindSet*>(textureArray)); }
    void VulkanGraph::SetConstant(NameID name, const void* data, uint32_t size) { m_globalResources.Set<char>(name, static_cast<const char*>(data), size); }
    void VulkanGraph::SetKeyword(NameID name, bool value) { m_globalResources.Set<bool>(name, value); }

    #undef PK_VK_BIND_HANDLES

    RHIBuffer* VulkanGraph::AcquireStagingBuffer(size_t size) { return m_stagingArena->BeginWrite(size); }
    void VulkanGraph::ReleaseStagingBuffer(RHIBuffer* buffer) { m_stagingArena->EndWrite(buffer); }


    void VulkanGraph::SetViewPorts(const uint4* rects, uint32_t count)
    {
        count = math::min(count, PK_RHI_MAX_VIEWPORTS);

        if (m_pipelineState.viewportCount != count)
        {
            m_pipelineState.viewportCount = count;
            m_pipelineState.dirtyFlags |= STATE_DIRTY_VIEWPORTS;
        }

        for (auto i = 0u; i < count && i < PK_RHI_MAX_VIEWPORTS; ++i)
        {
            auto& rect = rects[i];
            VkViewport v = { (float)rect.x, (float)rect.y, (float)rect.z, (float)rect.w, 0.0f, 1.0f };

            if (memcmp(&m_pipelineState.viewports[i], &v, sizeof(VkViewport)) != 0)
            {
                m_pipelineState.viewports[i] = v;
                m_pipelineState.dirtyFlags |= STATE_DIRTY_VIEWPORTS;
            }
        }
    }

    void VulkanGraph::SetScissors(const uint4* rects, uint32_t count)
    {
        count = math::min(count, PK_RHI_MAX_VIEWPORTS);

        if (m_pipelineState.scissorCount != count || memcmp(m_pipelineState.scissors, rects, sizeof(VkRect2D) * count) != 0)
        {
            memcpy(m_pipelineState.scissors, rects, sizeof(VkRect2D) * count);
            m_pipelineState.dirtyFlags |= STATE_DIRTY_SCISSORS;
            m_pipelineState.scissorCount = count;
        }
    }

    void VulkanGraph::SetRenderTarget(const RenderTargetBinding* bindings, uint32_t count, const uint4& renderArea, uint32_t layers)
    {
        VulkanRenderTarget renderTarget{};
        renderTarget.area = { { (int32_t)renderArea.x, (int32_t)renderArea.y}, { renderArea.z, renderArea.w } };
        renderTarget.layers = layers;
        renderTarget.colorCount = 0u;

        for (auto i = 0u; i < count; ++i)
        {
            auto binding = &bindings[i];
            auto target = binding->target->GetNativeView<VulkanBindHandle>(binding->targetRange, TextureViewMode::RTV);
            auto resolve = binding->resolve ? binding->resolve->GetNativeView<VulkanBindHandle>(binding->resolveRange, TextureViewMode::RTV) : nullptr;
            auto isDepth = VulkanEnumConvert::IsDepthFormat(target->image.format);
            auto attachment = isDepth ? &renderTarget.depth : (renderTarget.colors + renderTarget.colorCount++);
            attachment->target = target;
            attachment->resolve = resolve;
            attachment->loadOp = binding->loadOp;
            attachment->storeOp = binding->storeOp;
            attachment->clearValue = binding->clearValue;
            attachment->resolveMode = resolve ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
        }

        if (memcmp(&m_pipelineState.renderTarget, &renderTarget, sizeof(VulkanRenderTarget)) != 0)
        {
            m_pipelineState.dirtyFlags |= STATE_DIRTY_RENDERTARGET;
            m_pipelineState.renderTarget = renderTarget;
        }
    }

    void VulkanGraph::SetBlending(const BlendParameters& blend)
    {
        if (memcmp(&m_pipelineState.pipelineKey.fixed.blending, &blend, sizeof(BlendParameters)) != 0)
        {
            m_pipelineState.dirtyFlags |= STATE_DIRTY_PIPELINE;
            m_pipelineState.pipelineKey.fixed.blending = blend;
        }
    }

    void VulkanGraph::SetRasterization(const RasterizationParameters& rasterization)
    {
        if (memcmp(&m_pipelineState.pipelineKey.fixed.rasterization, &rasterization, sizeof(RasterizationParameters)) != 0)
        {
            m_pipelineState.dirtyFlags |= STATE_DIRTY_PIPELINE;
            m_pipelineState.pipelineKey.fixed.rasterization = rasterization;
        }
    }

    void VulkanGraph::SetDepthStencil(const DepthStencilParameters& depthStencil)
    {
        if (memcmp(&m_pipelineState.pipelineKey.fixed.depthStencil, &depthStencil, sizeof(DepthStencilParameters)) != 0)
        {
            m_pipelineState.dirtyFlags |= STATE_DIRTY_PIPELINE;
            m_pipelineState.pipelineKey.fixed.depthStencil = depthStencil;
        }
    }

    void VulkanGraph::SetMultisampling(const MultisamplingParameters& multisampling)
    {
        if (memcmp(&m_pipelineState.pipelineKey.fixed.multisampling, &multisampling, sizeof(MultisamplingParameters)) != 0)
        {
            m_pipelineState.dirtyFlags |= STATE_DIRTY_PIPELINE;
            m_pipelineState.pipelineKey.fixed.multisampling = multisampling;
        }
    }

    void VulkanGraph::SetStageExcludeMask(const ShaderStageFlags mask)
    {
        if (m_pipelineState.pipelineKey.fixed.excludeStageMask != (uint16_t)mask)
        {
            m_pipelineState.dirtyFlags |= STATE_DIRTY_PIPELINE;
            m_pipelineState.pipelineKey.fixed.excludeStageMask = (uint16_t)mask;
        }
    }

    void VulkanGraph::SetShader(const RHIShader* shader)
    {
        if (m_pipelineState.pipelineKey.shader != shader)
        {
            m_pipelineState.dirtyFlags |= STATE_DIRTY_PIPELINE | STATE_DIRTY_SHADER;
            m_pipelineState.pipelineKey.shader = static_cast<const VulkanShader*>(shader);
        }
    }

    void VulkanGraph::SetVertexBuffers(const RHIBuffer** buffers, uint32_t count)
    {
        auto i = 0u;

        for (; i < count && i < PK_RHI_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            auto handle = buffers[i]->GetNativeView<VulkanBindHandle>();
            PK_DEBUG_FATAL_ASSERT(handle != nullptr, "Passing null vertex buffer is not allowed!");

            if (handle != m_pipelineState.vertexBuffers[i])
            {
                m_pipelineState.dirtyFlags |= STATE_DIRTY_VERTEXBUFFERS;
                m_pipelineState.vertexBuffers[i] = handle;
            }
        }

        if (i < PK_RHI_MAX_VERTEX_ATTRIBUTES)
        {
            Memory::Memset(m_pipelineState.vertexBuffers + i, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - i);
        }
    }

    void VulkanGraph::SetVertexStreams(const VertexStreamElement* elements, uint32_t count)
    {
        PK_DEBUG_FATAL_ASSERT(count <= PK_RHI_MAX_VERTEX_ATTRIBUTES, "Tried to bind more vertex attributes than currently supported!");

        if (memcmp(m_pipelineState.vertexStreamLayout, elements, sizeof(VertexStreamElement) * count) != 0)
        {
            m_pipelineState.dirtyFlags |= STATE_DIRTY_VERTEXBUFFERS;
            memcpy(m_pipelineState.vertexStreamLayout, elements, sizeof(VertexStreamElement) * count);
        }

        if (count < PK_RHI_MAX_VERTEX_ATTRIBUTES && m_pipelineState.vertexStreamLayout[count].stride != 0)
        {
            Memory::Memset(m_pipelineState.vertexStreamLayout + count, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - count);
        }
    }

    void VulkanGraph::SetIndexBuffer(const RHIBuffer* buffer, size_t indexSize)
    {
        auto handle = buffer->GetNativeView<VulkanBindHandle>();
        auto indexType = VulkanEnumConvert::GetIndexType(indexSize);

        if (m_pipelineState.indexBuffer != handle || (handle != nullptr && m_pipelineState.indexType != indexType))
        {
            m_pipelineState.indexBuffer = handle;
            m_pipelineState.indexType = indexType;
        }
    }

    void VulkanGraph::SetShaderBindingTable(RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset, size_t stride, size_t size)
    {
        const auto address = buffer->GetDeviceAddress();
        m_pipelineState.sbtAddresses[(uint32_t)group] = { address + offset, stride, size };
    }


    void VulkanGraph::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanGraphContext* ctx, VulkanDrawCmd* cmd)
        {
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->viewports, cmd->viewportCount, cmd->scissors, cmd->scissorCount);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->pipelineLayout, cmd->descriptorSetOffset, cmd->pushConstants);
            ctx->cmb->SetVertexBuffers(cmd->vertexBuffers, cmd->vertexBufferOffsets, cmd->vertexBufferCount);
            vkCmdDraw(*ctx->cmb, cmd->vertexCount, cmd->instanceCount, cmd->firstVertex, cmd->firstInstance);
        });
            
        cmd->passIndex = AddBarrierPass(QueueType::Graphics, true);
        cmd->vertexCount = vertexCount;
        cmd->instanceCount = instanceCount;
        cmd->firstVertex = firstVertex;
        cmd->firstInstance = firstInstance;
        RecordVertexCmd(cmd);
    }

    void VulkanGraph::DrawIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanGraphContext* ctx, VulkanDrawIndirectCmd* cmd)
        {
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->viewports, cmd->viewportCount, cmd->scissors, cmd->scissorCount);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->pipelineLayout, cmd->descriptorSetOffset, cmd->pushConstants);
            ctx->cmb->SetVertexBuffers(cmd->vertexBuffers, cmd->vertexBufferOffsets, cmd->vertexBufferCount);
            vkCmdDrawIndirect(*ctx->cmb, cmd->indirectArguments, cmd->indirectArgumentsOffset, cmd->drawCount, cmd->stride);
        });

        auto indirectArgumentsHandle = indirectArguments->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(QueueType::Graphics, true);
        cmd->indirectArguments = indirectArgumentsHandle->buffer.buffer;
        cmd->indirectArgumentsOffset = offset + indirectArgumentsHandle->buffer.offset;
        cmd->drawCount = drawCount;
        cmd->stride = stride;
        RecordVertexCmd(cmd);
        RecordBufferAccess(QueueType::Graphics, indirectArgumentsHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanGraphContext* ctx, VulkanDrawIndexedCmd* cmd)
        {
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->viewports, cmd->viewportCount, cmd->scissors, cmd->scissorCount);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->pipelineLayout, cmd->descriptorSetOffset, cmd->pushConstants);
            ctx->cmb->SetVertexBuffers(cmd->vertexBuffers, cmd->vertexBufferOffsets, cmd->vertexBufferCount);
            ctx->cmb->SetIndexBuffer(cmd->indexBuffer, cmd->indexBufferOffset, cmd->indexType);
            vkCmdDrawIndexed(*ctx->cmb, cmd->indexCount, cmd->instanceCount, cmd->firstIndex, cmd->vertexOffset, cmd->firstInstance);
        });

        cmd->passIndex = AddBarrierPass(QueueType::Graphics, true);
        cmd->indexCount = indexCount;
        cmd->instanceCount = instanceCount;
        cmd->firstIndex = firstIndex;
        cmd->vertexOffset = vertexOffset;
        cmd->firstInstance = firstInstance;
        RecordIndexCmd(cmd);
    }

    void VulkanGraph::DrawIndexedIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanGraphContext* ctx, VulkanDrawIndexedIndirectCmd* cmd)
        {
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->viewports, cmd->viewportCount, cmd->scissors, cmd->scissorCount);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->pipelineLayout, cmd->descriptorSetOffset, cmd->pushConstants);
            ctx->cmb->SetVertexBuffers(cmd->vertexBuffers, cmd->vertexBufferOffsets, cmd->vertexBufferCount);
            ctx->cmb->SetIndexBuffer(cmd->indexBuffer, cmd->indexBufferOffset, cmd->indexType);
            vkCmdDrawIndexedIndirect(*ctx->cmb, cmd->indirectArguments, cmd->indirectArgumentsOffset, cmd->drawCount, cmd->stride);
        });

        auto indirectArgumentsHandle = indirectArguments->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(QueueType::Graphics, true);
        cmd->indirectArguments = indirectArgumentsHandle->buffer.buffer;
        cmd->indirectArgumentsOffset = offset + indirectArgumentsHandle->buffer.offset;
        cmd->drawCount = drawCount;
        cmd->stride = stride;
        RecordIndexCmd(cmd);
        RecordBufferAccess(QueueType::Graphics, indirectArgumentsHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::DrawMeshTasks(const uint3& dimensions)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanGraphContext* ctx, VulkanDrawMeshTasksCmd* cmd)
        {
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->viewports, cmd->viewportCount, cmd->scissors, cmd->scissorCount);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->pipelineLayout, cmd->descriptorSetOffset, cmd->pushConstants);
            vkCmdDrawMeshTasksEXT(*ctx->cmb, cmd->dimensions.x, cmd->dimensions.y, cmd->dimensions.z);
        });

        cmd->passIndex = AddBarrierPass(QueueType::Graphics, true);
        cmd->dimensions = dimensions;
        RecordRasterCmd(cmd);
    }

    void VulkanGraph::DrawMeshTasksIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanGraphContext* ctx, VulkanDrawMeshTasksIndirectCmd* cmd)
        {
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->viewports, cmd->viewportCount, cmd->scissors, cmd->scissorCount);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->pipelineLayout, cmd->descriptorSetOffset, cmd->pushConstants);
            vkCmdDrawMeshTasksIndirectEXT(*ctx->cmb, cmd->indirectArguments, cmd->indirectArgumentsOffset, cmd->drawCount, cmd->stride);
        });

        auto indirectArgumentsHandle = indirectArguments->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(QueueType::Graphics, true);
        cmd->indirectArguments = indirectArgumentsHandle->buffer.buffer;
        cmd->indirectArgumentsOffset = offset + indirectArgumentsHandle->buffer.offset;
        cmd->drawCount = drawCount;
        cmd->stride = stride;
        RecordRasterCmd(cmd);
        RecordBufferAccess(QueueType::Graphics, indirectArgumentsHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::DrawMeshTasksIndirectCount(const RHIBuffer* indirectArguments, size_t offset, const RHIBuffer* countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanGraphContext* ctx, VulkanDrawMeshTasksIndirectCountCmd* cmd)
        {
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->viewports, cmd->viewportCount, cmd->scissors, cmd->scissorCount);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->pipelineLayout, cmd->descriptorSetOffset, cmd->pushConstants);
            vkCmdDrawMeshTasksIndirectCountEXT(*ctx->cmb, cmd->indirectArguments, cmd->indirectArgumentsOffset, cmd->countBuffer, cmd->countBufferOffset, cmd->maxDrawCount, cmd->stride);
        });

        auto indirectArgumentsHandle = indirectArguments->GetNativeView<VulkanBindHandle>();
        auto countBufferHandle = countBuffer->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(QueueType::Graphics, true);
        cmd->indirectArguments = indirectArgumentsHandle->buffer.buffer;
        cmd->indirectArgumentsOffset = offset + indirectArgumentsHandle->buffer.offset;
        cmd->countBuffer = countBufferHandle->buffer.buffer;
        cmd->countBufferOffset = countOffset + countBufferHandle->buffer.offset;
        cmd->maxDrawCount = maxDrawCount;
        cmd->stride = stride;
        RecordRasterCmd(cmd);
        RecordBufferAccess(QueueType::Graphics, indirectArgumentsHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
        RecordBufferAccess(QueueType::Graphics, countBufferHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::Dispatch(QueueType queue, const uint3& dimensions)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanDispatchCmd* cmd)
        {
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->pipelineLayout, cmd->descriptorSetOffset, cmd->pushConstants);
            vkCmdDispatch(*ctx->cmb, cmd->groupCounts.x, cmd->groupCounts.y, cmd->groupCounts.z);
        });

        const auto groupSize = m_pipelineState.pipelineKey.shader->GetGroupSize();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->groupCounts.x = (dimensions.x + groupSize.x - 1u) / groupSize.x;
        cmd->groupCounts.y = (dimensions.y + groupSize.y - 1u) / groupSize.y;
        cmd->groupCounts.z = (dimensions.z + groupSize.z - 1u) / groupSize.z;
        RecordShaderCmd(queue, cmd);
    }

    void VulkanGraph::DispatchIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanDispatchIndirectCmd* cmd)
        {
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->pipelineLayout, cmd->descriptorSetOffset, cmd->pushConstants);
            vkCmdDispatchIndirect(*ctx->cmb, cmd->indirectArguments, cmd->indirectArgumentsOffset);
        });

        auto indirectArgumentsHandle = indirectArguments->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->indirectArguments = indirectArgumentsHandle->buffer.buffer;
        cmd->indirectArgumentsOffset = offset + indirectArgumentsHandle->buffer.offset;
        RecordShaderCmd(queue, cmd);
        RecordBufferAccess(queue, indirectArgumentsHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::DispatchRays(QueueType queue, const uint3& dimensions)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanDispatchRaysCmd* cmd)
        {
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->pipelineLayout, cmd->descriptorSetOffset, cmd->pushConstants);
            vkCmdTraceRaysKHR(*ctx->cmb, &cmd->addressRayGen, &cmd->addressMiss, &cmd->addressHit, &cmd->addressCallable, cmd->dimensions.x, cmd->dimensions.y, cmd->dimensions.z);
        });

        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->addressRayGen = m_pipelineState.sbtAddresses[(uint32_t)RayTracingShaderGroup::RayGeneration];
        cmd->addressMiss = m_pipelineState.sbtAddresses[(uint32_t)RayTracingShaderGroup::Miss];
        cmd->addressHit = m_pipelineState.sbtAddresses[(uint32_t)RayTracingShaderGroup::Hit];
        cmd->addressCallable = m_pipelineState.sbtAddresses[(uint32_t)RayTracingShaderGroup::Callable];
        cmd->dimensions = dimensions;
        RecordShaderCmd(queue, cmd);
    }

    void VulkanGraph::Blit(QueueType queue, RHITexture* src, RHISwapchain* dst, FilterMode filter)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanBlitCmd* cmd)
        {
            auto handle = cmd->dstSwapchain->GetBindHandle();
            auto signal = cmd->dstSwapchain->ConsumeAcquireSignal();

            if (signal != VK_NULL_HANDLE)
            {
                ctx->queue->AddWait(signal, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 0ull);
            }

            VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            barrier.srcAccessMask = 0u;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = handle->image.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0u;
            barrier.subresourceRange.baseMipLevel = 0u;
            barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

            cmd->dst = handle->image.image;
            cmd->region.dstSubresource = { (uint32_t)handle->image.range.aspectMask, 0u, 0u, 1u };
            ctx->cmb->Barrier(&barrier);
            vkCmdBlitImage(*ctx->cmb, cmd->src, VK_IMAGE_LAYOUT_GENERAL, handle->image.image, VK_IMAGE_LAYOUT_GENERAL, 1, &cmd->region, cmd->filter);
        });

        const auto& handle = src->GetNativeView<VulkanBindHandle>({}, TextureViewMode::RTV);
        auto srcRes = src->GetResolution();
        auto dstRes = dst->GetResolution();
        auto minres = math::min(srcRes, dstRes);
        auto diff = int3(srcRes - minres);
        auto srcMin = diff / 2;
        auto srcMax = int3(srcRes) - (diff - srcMin);

        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->src = handle->image.image;
        cmd->dstSwapchain = static_cast<VulkanSwapchain*>(dst);
        cmd->region.srcSubresource = { (uint32_t)handle->image.range.aspectMask, 0u, 0u, 1u };
        cmd->region.srcOffsets[0] = { srcMin.x, srcMax.y, srcMin.z };
        cmd->region.srcOffsets[1] = { srcMax.x, srcMin.y, srcMax.z };
        cmd->region.dstOffsets[0] = { 0, 0, 0 };
        cmd->region.dstOffsets[1] = { (int)dstRes.x, (int)dstRes.y, (int)dstRes.z };
        cmd->filter = VulkanEnumConvert::GetFilterMode(filter);

        RecordImageAccess(queue, handle, VK_PIPELINE_STAGE_2_BLIT_BIT_KHR, VK_ACCESS_2_TRANSFER_READ_BIT);
    }

    void VulkanGraph::Blit(QueueType queue, RHISwapchain* src, RHIBuffer* dst)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanBlitSwapchainToBufferCmd* cmd)
        {
            auto handle = cmd->src->GetBindHandle();
            auto signal = cmd->src->ConsumeAcquireSignal();

            if (signal != VK_NULL_HANDLE)
            {
                ctx->queue->AddWait(signal, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 0ull);
            }

            VkImageMemoryBarrier2 barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            barrier.oldLayout = signal ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = handle->image.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0u;
            barrier.subresourceRange.baseMipLevel = 0u;
            barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

            cmd->region.imageOffset = { 0,0,0 };
            cmd->region.imageExtent = handle->image.extent;
            cmd->region.imageSubresource.aspectMask = handle->image.range.aspectMask;
            cmd->region.imageSubresource.mipLevel = 0u;
            cmd->region.imageSubresource.baseArrayLayer = 0u;
            cmd->region.imageSubresource.layerCount = VK_REMAINING_ARRAY_LAYERS;

            ctx->cmb->Barrier(&barrier);
            vkCmdCopyImageToBuffer(*ctx->cmb, handle->image.image, VK_IMAGE_LAYOUT_GENERAL, cmd->dst, 1, &cmd->region);
        });

        auto handle = dst->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->src = static_cast<VulkanSwapchain*>(src);
        cmd->dst = handle->buffer.buffer;
        cmd->region.bufferOffset = handle->buffer.offset;
        RecordBufferAccess(queue, handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    }

    void VulkanGraph::Blit(QueueType queue, RHITexture* src, RHITexture* dst, const TextureViewRange& srcRange, const TextureViewRange& dstRange, FilterMode filter)
    {
        BeginDebugScope("Blit Image", PK_COLOR_RED);

        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanBlitCmd* cmd)
        {
            const auto srcBlockSize = VulkanEnumConvert::GetFormatBlockSize(cmd->srcFormat);
            const auto dstBlockSize = VulkanEnumConvert::GetFormatBlockSize(cmd->dstFormat);
            auto useCopy = srcBlockSize == dstBlockSize;
            useCopy &= cmd->region.srcOffsets[1].x - cmd->region.srcOffsets[0].x == cmd->region.dstOffsets[1].x - cmd->region.dstOffsets[0].x;
            useCopy &= cmd->region.srcOffsets[1].y - cmd->region.srcOffsets[0].y == cmd->region.dstOffsets[1].y - cmd->region.dstOffsets[0].y;
            useCopy &= cmd->region.srcOffsets[1].z - cmd->region.srcOffsets[0].z == cmd->region.dstOffsets[1].z - cmd->region.dstOffsets[0].z;
            useCopy &= cmd->region.srcSubresource.mipLevel == cmd->region.dstSubresource.mipLevel;
            useCopy &= cmd->region.srcSubresource.layerCount == cmd->region.dstSubresource.layerCount;

            if (cmd->srcSamples > VK_SAMPLE_COUNT_1_BIT && cmd->dstSamples == VK_SAMPLE_COUNT_1_BIT)
            {
                VkImageResolve resolveRegion{};
                resolveRegion.srcSubresource = { cmd->region.srcSubresource.aspectMask, cmd->region.srcSubresource.mipLevel, cmd->region.srcSubresource.baseArrayLayer, cmd->region.srcSubresource.layerCount };
                resolveRegion.dstSubresource = { cmd->region.dstSubresource.aspectMask, cmd->region.dstSubresource.mipLevel, cmd->region.dstSubresource.baseArrayLayer, cmd->region.dstSubresource.layerCount };
                resolveRegion.extent.width = (uint32_t)cmd->region.dstOffsets[1].x - (uint32_t)cmd->region.dstOffsets[0].x;
                resolveRegion.extent.height = (uint32_t)cmd->region.dstOffsets[1].y - (uint32_t)cmd->region.dstOffsets[0].y;
                resolveRegion.extent.depth = (uint32_t)cmd->region.dstOffsets[1].z - (uint32_t)cmd->region.dstOffsets[0].z;
                vkCmdResolveImage(*ctx->cmb, cmd->src, VK_IMAGE_LAYOUT_GENERAL, cmd->dst, VK_IMAGE_LAYOUT_GENERAL, 1, &resolveRegion);
            }
            else if (useCopy)
            {
                VkImageCopy copyRegion;
                copyRegion.srcSubresource = cmd->region.srcSubresource;
                copyRegion.srcOffset = cmd->region.srcOffsets[0];
                copyRegion.dstSubresource = cmd->region.dstSubresource;
                copyRegion.dstOffset = cmd->region.srcOffsets[0];
                copyRegion.extent.width = (uint32_t)cmd->region.dstOffsets[1].x - (uint32_t)cmd->region.dstOffsets[0].x;
                copyRegion.extent.height = (uint32_t)cmd->region.dstOffsets[1].y - (uint32_t)cmd->region.dstOffsets[0].y;
                copyRegion.extent.depth = (uint32_t)cmd->region.dstOffsets[1].z - (uint32_t)cmd->region.dstOffsets[0].z;
                vkCmdCopyImage(*ctx->cmb, cmd->src, VK_IMAGE_LAYOUT_GENERAL, cmd->dst, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
            }
            else
            {
                vkCmdBlitImage(*ctx->cmb, cmd->src, VK_IMAGE_LAYOUT_GENERAL, cmd->dst, VK_IMAGE_LAYOUT_GENERAL, 1, &cmd->region, cmd->filter);
            }
        });

        auto srcHandle = src->GetNativeView<VulkanBindHandle>(srcRange, TextureViewMode::RTV);
        auto dstHandle = dst->GetNativeView<VulkanBindHandle>(dstRange, TextureViewMode::RTV);
        auto srcLayers = math::min(srcHandle->image.range.layerCount, src->GetLayers());
        auto dstLayers = math::min(dstHandle->image.range.layerCount, dst->GetLayers());

        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->src = srcHandle->image.image;
        cmd->dst = dstHandle->image.image;
        cmd->srcFormat = srcHandle->image.format;
        cmd->dstFormat = dstHandle->image.format;
        cmd->srcSamples = srcHandle->image.samples;
        cmd->dstSamples = dstHandle->image.samples;
        cmd->region.srcSubresource = { (uint32_t)srcHandle->image.range.aspectMask, srcHandle->image.range.baseMipLevel, srcHandle->image.range.baseArrayLayer, 0u };
        cmd->region.dstSubresource = { (uint32_t)srcHandle->image.range.aspectMask, dstHandle->image.range.baseMipLevel, dstHandle->image.range.baseArrayLayer, 0u };
        cmd->region.srcOffsets[1] = { (int)srcHandle->image.extent.width, (int)srcHandle->image.extent.height, (int)srcHandle->image.extent.depth };
        cmd->region.dstOffsets[1] = { (int)dstHandle->image.extent.width, (int)dstHandle->image.extent.height, (int)dstHandle->image.extent.depth };
        cmd->region.dstSubresource.layerCount = cmd->region.srcSubresource.layerCount = math::min(srcLayers, dstLayers);
        cmd->filter = VulkanEnumConvert::GetFilterMode(filter);
        
        RecordImageAccess(queue, srcHandle, VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
        RecordImageAccess(queue, dstHandle, VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
        EndDebugScope();
    }

    void VulkanGraph::Clear(QueueType queue, RHIBuffer* dst, size_t offset, size_t size, uint32_t value)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanFillBufferCmd* cmd)
        {
            vkCmdFillBuffer(*ctx->cmb, cmd->buffer, cmd->offset, cmd->size, cmd->value);
        });

        auto handle = dst->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->buffer = handle->buffer.buffer;
        cmd->offset = offset + handle->buffer.offset;
        cmd->size = size;
        cmd->value = value;
        RecordBufferAccess(queue, handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    }

    void VulkanGraph::Clear(QueueType queue, RHITexture* dst, const TextureViewRange& range, const TextureClearValue& value)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanClearImageCmd* cmd)
        {
            if (cmd->range.aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
            {
                vkCmdClearDepthStencilImage(*ctx->cmb, cmd->image, VK_IMAGE_LAYOUT_GENERAL, &cmd->clearValue.depthStencil, 1, &cmd->range);
            }
            else
            {
                vkCmdClearColorImage(*ctx->cmb, cmd->image, VK_IMAGE_LAYOUT_GENERAL, &cmd->clearValue.color, 1, &cmd->range);
            }
        });

        auto handle = dst->GetNativeView<VulkanBindHandle>(range, TextureViewMode::UAV);
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->image = handle->image.image;
        cmd->range = handle->image.range;
        cmd->clearValue = VulkanEnumConvert::GetClearValue(value);
        RecordImageAccess(queue, handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT);
    }

    void VulkanGraph::UpdateBuffer(QueueType queue, RHIBuffer* dst, size_t offset, size_t size, const void* data)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanUpdateBufferCmd* cmd)
        {
            vkCmdUpdateBuffer(*ctx->cmb, cmd->buffer, cmd->offset, cmd->size, cmd->pData);
        });

        auto handle = dst->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->buffer = handle->buffer.buffer;
        cmd->offset = offset + handle->buffer.offset;
        cmd->size = size;
        cmd->pData = Allocate<uint8_t>(size);
        memcpy(cmd->pData, data, size);
        RecordBufferAccess(queue, handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT);
    }

    void VulkanGraph::CopyBuffer(QueueType queue, RHIBuffer* dst, RHIBuffer* src, size_t srcOffset, size_t dstOffset, size_t size)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanCopyBufferCmd* cmd)
        {
            vkCmdCopyBuffer(*ctx->cmb, cmd->src, cmd->dst, 1, &cmd->region);
        });

        auto srcHandle = src->GetNativeView<VulkanBindHandle>();
        auto dstHandle = dst->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->src = srcHandle->buffer.buffer;
        cmd->dst = dstHandle->buffer.buffer;
        cmd->region.srcOffset = srcOffset + srcHandle->buffer.offset;
        cmd->region.dstOffset = dstOffset + dstHandle->buffer.offset;
        cmd->region.size = size;

        // Assume always staging source?!?
        RecordBufferAccess(queue, dstHandle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    }

    void VulkanGraph::CopyToTexture(QueueType queue, RHITexture* texture, RHIBuffer* buffer, TextureDataRegion* regions, uint32_t regionCount)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanCopyBufferToImageCmd* cmd)
        {
            vkCmdCopyBufferToImage(*ctx->cmb, cmd->src, cmd->dst, VK_IMAGE_LAYOUT_GENERAL, cmd->regionCount, cmd->regions);
        });

        auto resourceRange = TextureViewRange();
        resourceRange.level = 0xFFFFu;
        resourceRange.layer = 0xFFFFu;

        for (auto i = 0u; i < regionCount; ++i)
        {
            auto& region = regions[i];
            resourceRange.level = math::min((uint32_t)resourceRange.level, region.level);
            resourceRange.layer = math::min((uint32_t)resourceRange.layer, region.layer);
            resourceRange.levels = math::max((uint32_t)resourceRange.levels, region.level + 1u);
            resourceRange.layers = math::max((uint32_t)resourceRange.layers, region.layer + region.layers);
        }

        resourceRange.levels = resourceRange.levels - resourceRange.level;
        resourceRange.layers = resourceRange.layers - resourceRange.layer;

        auto srcHandle = buffer->GetNativeView<VulkanBindHandle>();
        auto dstHandle = texture->GetNativeView<VulkanBindHandle>(resourceRange, TextureViewMode::SRV);

        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->src = srcHandle->buffer.buffer;
        cmd->dst = dstHandle->image.image;
        cmd->regions = Allocate<VkBufferImageCopy>(regionCount);
        cmd->regionCount = regionCount;

        for (auto i = 0u; i < regionCount; ++i)
        {
            auto& region = regions[i];
            cmd->regions[i].bufferOffset = region.bufferOffset + srcHandle->buffer.offset;
            cmd->regions[i].bufferRowLength = 0u;
            cmd->regions[i].bufferImageHeight = 0u;
            cmd->regions[i].imageSubresource.aspectMask = dstHandle->image.range.aspectMask;
            cmd->regions[i].imageSubresource.mipLevel = region.level;
            cmd->regions[i].imageSubresource.baseArrayLayer = region.layer;
            cmd->regions[i].imageSubresource.layerCount = region.layers;
            cmd->regions[i].imageOffset.x = region.offset.x;
            cmd->regions[i].imageOffset.y = region.offset.y;
            cmd->regions[i].imageOffset.z = region.offset.z;
            cmd->regions[i].imageExtent.width = region.extent.x;
            cmd->regions[i].imageExtent.height = region.extent.y;
            cmd->regions[i].imageExtent.depth = region.extent.z;
        }
        
        // Assume always staging source?!?
        RecordImageAccess(queue, dstHandle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    }

    RHIAccelerationStructureBuilder* VulkanGraph::BeginAccelerationStructureWrite(RHIAccelerationStructure* structure, size_t instanceLimit)
    {
        auto vkStructure = static_cast<VulkanAccelerationStructure*>(structure);
        auto writer = AllocateNew<VulkanAccelerationStructureBuilder>();
        auto inputBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * instanceLimit;
        writer->structure = vkStructure;
        writer->stagingBuffer = AcquireStagingBuffer(inputBufferSize);
        writer->instances = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(writer->stagingBuffer->BeginMap(0ull, 0ull));
        writer->instanceIndices = Allocate<uint32_t>(instanceLimit);
        writer->instanceCount = 0u;
        writer->instanceLimit = instanceLimit;
        writer->topologyHash = 0ull;
        return writer;
    }

    void VulkanGraph::EndAccelerationStructureWrite(QueueType queue, RHIAccelerationStructureBuilder* builder)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanWriteAccelerationStructureCmd* cmd)
        {
            for (auto i = 0u; i < cmd->copyCount; ++i)
            {
                vkCmdCopyAccelerationStructureKHR(*ctx->cmb, &cmd->copyInfos[i]);
            }

            if (cmd->BLASBuildCount)
            {
                vkCmdBuildAccelerationStructuresKHR(*ctx->cmb, cmd->BLASBuildCount, cmd->BLASBuildInfos, cmd->BLASRangeInfos);
            }

            if (cmd->BLASBuildCount || cmd->copyCount)
            {
                VkMemoryBarrier2 memoryBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER_2 };
                memoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
                memoryBarrier.srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
                memoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
                memoryBarrier.dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
                ctx->cmb->Barrier(&memoryBarrier, nullptr, 0u);
            }

            if (cmd->queryCount)
            {
                vkCmdWriteAccelerationStructuresPropertiesKHR(*ctx->cmb, cmd->queryCount, cmd->queryHandles, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, cmd->queryPool, cmd->queryStart);
            }

            if (cmd->TLASBuild)
            {
                const auto* pRangeInfo = &cmd->TLASRangeInfo;
                vkCmdBuildAccelerationStructuresKHR(*ctx->cmb, 1u, &cmd->TLASBuildInfo, &pRangeInfo);
            }
        });

        auto vkBuilder = static_cast<VulkanAccelerationStructureBuilder*>(builder);
        auto vkStructure = vkBuilder->structure;
        auto queryPool = vkStructure->queryPool.get();
        const auto substructureCount = vkStructure->substructures.GetCount();

        PK_DEBUG_WARNING_ASSERT(vkBuilder->instanceCount, "VulkanAccelerationStructure.EndWrite: write has 0 instances!");

        const bool hasCompactedResults = vkStructure->queryCount && queryPool->WaitResults(0ull);

        if (hasCompactedResults)
        {
            PK_LOG_RHI("Bottom Level Compaction Update: %s", vkStructure->name.c_str());
            PK_LOG_INDENT(PK_LOG_LVL_RHI);

            for (auto i = 0u; i < substructureCount; ++i)
            {
                auto structure = &vkStructure->substructures[i].value;
                if (structure->handle && structure->compactionId && structure->compactionId != VulkanAccelerationStructure::COMPACTED_ID)
                {
                    const auto size0 = structure->size;
                    const auto size1 = queryPool->GetResult<VkDeviceSize>(structure->compactionId - 1u, VK_QUERY_RESULT_64_BIT);
                    structure->size = size1;
                    PK_LOG_RHI("BLAS Compacted from %i to %i bytes", size0, size1);
                }
            }

            queryPool->ResetQuery(0u, vkStructure->queryCount);
            vkStructure->queryCount = 0u;
        }

        VkDeviceSize bufferSize = 0ull;
        VkDeviceSize scratchSize = 0ull;
        VkDeviceSize buildCount = 0ull;

        for (auto i = 0u; i < substructureCount; ++i)
        {
            auto structure = &vkStructure->substructures[i].value;
            structure->bufferOffset = bufferSize;
            bufferSize += math::align(structure->size, 256ull);

            if (!structure->handle)
            {
                structure->scratchOffset = scratchSize;
                scratchSize += math::align(structure->buildScratchSize, 256ull);
                ++buildCount;
            }
        }

        auto* tlasGeometry = Allocate<VkAccelerationStructureGeometryKHR>(1);
        auto buildInfo = VulkanAccelerationStructure::GetTLASBuildInfo(vkBuilder->stagingBuffer->GetDeviceAddress(), tlasGeometry);
        const auto sizeInfo = VulkanGetAccelerationBuildSizesInfo(m_driver->device, buildInfo, vkBuilder->instanceCount);

        const auto prevSize = vkStructure->structure.size;
        vkStructure->structure.size = sizeInfo.accelerationStructureSize;
        vkStructure->structure.buildScratchSize = sizeInfo.buildScratchSize;
        vkStructure->structure.bufferOffset = bufferSize;
        vkStructure->structure.scratchOffset = scratchSize;
        bufferSize += math::align(sizeInfo.accelerationStructureSize, 256ull);
        scratchSize += math::align(sizeInfo.buildScratchSize, 256ull);

        const auto rebuildBLAS = buildCount ||
            hasCompactedResults ||
            prevSize < sizeInfo.accelerationStructureSize ||
            vkStructure->buffer == nullptr ||
            vkStructure->buffer->GetSize() < bufferSize;

        const auto rebuildTLAS = rebuildBLAS || vkStructure->topologyHash != vkBuilder->topologyHash;

        auto needsScratch = (buildCount || rebuildTLAS) && scratchSize;
        auto scratchBuffer = needsScratch ? AcquireStagingBuffer(scratchSize) : nullptr;

        if (rebuildBLAS)
        {
            PK_LOG_RHI_SCOPE("Acceleration Structure Update: %s", vkStructure->name.c_str());

            FixedString128 name({ vkStructure->name.c_str(), ".StructureBuffer" });
            vkStructure->buffer = RHI::CreateBuffer(bufferSize, BufferUsage::DefaultAccelerationStructure, name.c_str());

            cmd->BLASBuildInfos = Allocate<VkAccelerationStructureBuildGeometryInfoKHR>(buildCount);
            cmd->BLASRangeInfos = Allocate<VkAccelerationStructureBuildRangeInfoKHR*>(buildCount);
            cmd->copyInfos = Allocate<VkCopyAccelerationStructureInfoKHR>(substructureCount);

            for (auto i = 0u; i < substructureCount; ++i)
            {
                auto structure = &vkStructure->substructures[i].value;
                auto newHandle = vkStructure->CreateStructure(structure, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, FixedString128({ structure->name.c_str(), ".BLAS" }));

                if (structure->handle == VK_NULL_HANDLE)
                {
                    auto* geometry = Allocate<VkAccelerationStructureGeometryKHR>(1);
                    auto* rangeInfo = Allocate<VkAccelerationStructureBuildRangeInfoKHR>(1);
                    cmd->BLASBuildInfos[cmd->BLASBuildCount] = VulkanAccelerationStructure::GetBLASBuildInfo(structure->geometry, geometry, rangeInfo);
                    cmd->BLASBuildInfos[cmd->BLASBuildCount].dstAccelerationStructure = newHandle;
                    cmd->BLASBuildInfos[cmd->BLASBuildCount].scratchData.deviceAddress = scratchBuffer->GetDeviceAddress() + structure->scratchOffset;
                    cmd->BLASRangeInfos[cmd->BLASBuildCount++] = rangeInfo;
                }
                else
                {
                    auto& copyInfo = cmd->copyInfos[cmd->copyCount++];
                    copyInfo = VkCopyAccelerationStructureInfoKHR{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };
                    copyInfo.src = structure->handle;
                    copyInfo.dst = newHandle;
                    copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR;

                    if (hasCompactedResults && structure->compactionId && structure->compactionId != VulkanAccelerationStructure::COMPACTED_ID)
                    {
                        structure->compactionId = VulkanAccelerationStructure::COMPACTED_ID;
                        copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
                    }

                    vkStructure->DisposeStructure(structure->handle, GetFenceRef());
                }

                structure->deviceAddress = VulkanGetAccelerationStructureDeviceAddress(m_driver->device, newHandle);
                structure->handle = newHandle;
            }

            vkStructure->DisposeStructure(vkStructure->structure.handle, GetFenceRef());
            vkStructure->structure.handle = vkStructure->CreateStructure(&vkStructure->structure, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, FixedString128({ vkStructure->name.c_str(), ".TLAS" }));
            vkStructure->structure.deviceAddress = VulkanGetAccelerationStructureDeviceAddress(m_driver->device, vkStructure->structure.handle);
            vkStructure->bindHandle.acceleration.deviceAddress = vkStructure->structure.deviceAddress;
            vkStructure->bindHandle.IncrementVersion();
        }

        cmd->queryPool = queryPool->pool;
        cmd->queryHandles = Allocate<VkAccelerationStructureKHR>(substructureCount);
        cmd->queryStart = vkStructure->queryCount;

        for (auto i = 0u; i < substructureCount && vkStructure->queryCount < PK_VK_MAX_AS_COMPACTIONS; ++i)
        {
            auto structure = &vkStructure->substructures[i].value;

            if (structure->handle && !structure->compactionId)
            {
                queryPool->SetFence(GetFenceRef());
                cmd->queryHandles[cmd->queryCount++] = structure->handle;
                structure->compactionId = ++vkStructure->queryCount;
            }
        }

        if (rebuildTLAS)
        {
            for (auto i = 0u; i < vkBuilder->instanceCount; ++i)
            {
                auto index = vkBuilder->instanceIndices[i];
                vkBuilder->instances[i].accelerationStructureReference = vkStructure->substructures[index].value.deviceAddress;
            }

            vkStructure->lastBuildFenceRef = GetFenceRef();
            cmd->TLASBuildInfo = buildInfo;
            cmd->TLASBuildInfo.dstAccelerationStructure = vkStructure->structure.handle;
            cmd->TLASBuildInfo.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress() + vkStructure->structure.scratchOffset;
            cmd->TLASRangeInfo = VkAccelerationStructureBuildRangeInfoKHR{ vkBuilder->instanceCount, 0u, 0u, 0u };
            cmd->TLASBuild = true;
        }

        if (scratchBuffer)
        {
            ReleaseStagingBuffer(scratchBuffer);
        }

        ReleaseStagingBuffer(vkBuilder->stagingBuffer);
        vkStructure->topologyHash = vkBuilder->topologyHash;
        vkStructure->instanceCount = vkBuilder->instanceCount;

        cmd->passIndex = AddBarrierPass(queue, false);
    }


    void VulkanGraph::AcquireNextImage(RHISwapchain* swapchain)
    {
        auto cmd = AllocateCommand(QueueType::EnumCount, []([[maybe_unused]] VulkanGraphContext* ctx, VulkanAcquireNextImageCmd* cmd)
        {
            cmd->swapchain->AcquireNextImage();
        });

        cmd->swapchain = static_cast<VulkanSwapchain*>(swapchain);
        cmd->swapchain->Validate();
    }

    void VulkanGraph::Present(RHISwapchain* swapchain)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanGraphContext* ctx, VulkanPresentImageCmd* cmd)
        {
            auto handle = cmd->swapchain->GetBindHandle();
            auto acquireSignal = cmd->swapchain->ConsumeAcquireSignal();
            auto presentSignal = cmd->swapchain->ConsumePresentSignal();

            VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
            barrier.srcStageMask = acquireSignal ? VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.srcAccessMask = acquireSignal ? 0u : VK_ACCESS_2_TRANSFER_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            barrier.dstAccessMask = 0u;
            barrier.oldLayout = acquireSignal ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = handle->image.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0u;
            barrier.subresourceRange.baseMipLevel = 0u;
            barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            ctx->cmb->Barrier(&barrier);

            if (acquireSignal != VK_NULL_HANDLE)
            {
                ctx->queue->AddWait(acquireSignal, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 0ull);
            }

            if (presentSignal != VK_NULL_HANDLE)
            {
                ctx->graph->SubmitQueue(ctx->queueIndex, presentSignal, ctx->debugScopes);
                cmd->swapchain->Present(ctx->queue, presentSignal);
            }
        });

        cmd->swapchain = static_cast<VulkanSwapchain*>(swapchain);
    }


    void VulkanGraph::BeginDebugScope(const char* name, const color& color)
    {
        if (vkCmdBeginDebugUtilsLabelEXT)
        {
            auto cmd = AllocateCommand(QueueType::EnumCount, [](VulkanGraphContext* ctx, VulkanDebugCmd* cmd)
            {
                ctx->debugScopes->debugStack[ctx->debugScopes->debugHead++] = cmd;
            });

            auto length = strlen(name);
            cmd->color = color;
            cmd->label = Allocate<char>(length + 1u);
            strncpy(cmd->label, name, length);
        }
    }

    void VulkanGraph::EndDebugScope()
    {
        if (vkCmdEndDebugUtilsLabelEXT)
        {
            AllocateCommand(QueueType::EnumCount, [](VulkanGraphContext* ctx, [[maybe_unused]] VulkanCmd* cmd)
            {
                const auto head = --ctx->debugScopes->debugHead;
                ctx->debugScopes->debugStack[head] = nullptr;

                for (auto& min : ctx->debugScopes->debugMin)
                {
                    min = math::min(min, head);
                }
            });
        }
    }

    void VulkanGraph::BeginTimer(NameID name)
    {
        auto cmd = AllocateCommand(QueueType::EnumCount, [](VulkanGraphContext* ctx, VulkanTimerCmd* cmd)
        {
            ctx->debugScopes->timerStack[ctx->debugScopes->timerHead++] = cmd;
        });

        cmd->name = name;
    }

    void VulkanGraph::EndTimer()
    {
        AllocateCommand(QueueType::EnumCount, [](VulkanGraphContext* ctx, [[maybe_unused]] VulkanTimerCmd* cmd)
        {
            const auto head = --ctx->debugScopes->timerHead;
            ctx->debugScopes->timerStack[head] = nullptr;

            for (auto& min : ctx->debugScopes->timerMin)
            {
                min = math::min(min, head);
            }
        });
    }

    void VulkanGraph::AddDispatchHint(QueueType queue)
    {
        PK_FATAL_ERROR("NOT IMPLEMENTED!");
    }


    ConstBufferView<RHITimerScope> VulkanGraph::GetTimers() const { return m_timerArena->GetResults(); }

    FenceRef VulkanGraph::GetFenceRef() const
    {
        return FenceRef(this, [](const void* ctx, uint64_t userdata, uint64_t timeout)
        {
            auto graph = static_cast<const VulkanGraph*>(ctx);
            auto& scope = graph->m_scopes[userdata % PK_VK_MAX_GRAPH_SCOPES];

            if (userdata >= graph->m_invocationCounter)
            {
                return false;
            }

            if (scope.invocationIndex != userdata)
            {
                return scope.invocationIndex > userdata;
            }

            if (scope.fence.IsValid())
            {
                return scope.fence.Wait(timeout);
            }

            return true;
        },
        m_invocationCounter);
    }

    void VulkanGraph::Execute()
    {
        auto queues = m_driver->queues.get();
        auto queueCount = queues->GetQueueCount();
        auto& scope = EndGraph();
        m_timerArena->BeginTimeline();

        uint32_t activeQueueIndex = ~0u;
        uint32_t* activeDependencyMask = nullptr;
        VkPipelineStageFlags2* activeWaitStageMask = nullptr;
        uint32_t queueBatchId[(uint32_t)QueueType::EnumCount]{ 1u, 1u, 1u, 1u };
        uint32_t syncedBatchId[(uint32_t)QueueType::EnumCount][(uint32_t)QueueType::EnumCount]{};
        VulkanGraphDebugScopes debugScopes{};

        for (auto command = static_cast<VulkanCmd*>(GetCommands()); command; command = static_cast<VulkanCmd*>(command->next))
        {
            if (command->queue != QueueType::EnumCount)
            {
                const auto dstQueueIndex = queues->GetQueueIndex(command->queue);

                if (activeQueueIndex != dstQueueIndex)
                {
                    activeQueueIndex = dstQueueIndex;
                    activeDependencyMask = &command->dependencyMask;
                    activeWaitStageMask = &command->waitStageMask;
                    queueBatchId[dstQueueIndex]++;
                }

                if (command->passIndex)
                {
                    auto dependencies = m_accessRecorder.GetPassDependencies(command->passIndex);
                    auto rawMask = dependencies.dependencyMask & ~(1u << dstQueueIndex);
                    auto newMask = 0u;

                    for (auto srcQueueIndex = 0u; srcQueueIndex < queueCount; ++srcQueueIndex)
                    {
                        if ((rawMask & (1u << srcQueueIndex)) && syncedBatchId[dstQueueIndex][srcQueueIndex] < queueBatchId[srcQueueIndex])
                        {
                            newMask |= (1u << srcQueueIndex);
                            syncedBatchId[dstQueueIndex][srcQueueIndex] = queueBatchId[srcQueueIndex];
                        }
                    }

                    if (newMask)
                    {
                        *activeDependencyMask |= newMask;
                        *activeWaitStageMask |= dependencies.memoryBarrier->dstStageMask;
                    }

                    command->memoryBarrier = dependencies.memoryBarrier;
                    command->imageTransitions = dependencies.imageTransitions;
                    command->imageTransitionCount = dependencies.imageTransitionCount;
                }
            }
        }

        for (auto command = static_cast<VulkanCmd*>(GetCommands()); command; command = static_cast<VulkanCmd*>(command->next))
        {
            VulkanGraphContext ctx{};
            ctx.graph = this;
            ctx.debugScopes = &debugScopes;

            if (command->queue == QueueType::EnumCount)
            {
                command->call(&ctx, command);
                continue;
            }

            ctx.queueIndex = queues->GetQueueIndex(command->queue);
            ctx.queue = queues->GetQueueAt(ctx.queueIndex);
            
            if (command->dependencyMask)
            {
                SubmitQueue(ctx.queueIndex, VK_NULL_HANDLE, &debugScopes);

                for (auto srcQueueIndex = 0u; srcQueueIndex < queueCount; ++srcQueueIndex)
                {
                    if (command->dependencyMask & (1u << srcQueueIndex))
                    {
                        SubmitQueue(srcQueueIndex, VK_NULL_HANDLE, &debugScopes);
                        ctx.queue->AddWait(queues->GetQueueAt(srcQueueIndex), command->waitStageMask);
                    }
                }
            }

            ctx.cmb = ctx.queue->GetCommandBuffer();
            ctx.cmb->PrepareForCommands();

            ApplyDebugScopes(ctx.queueIndex, &debugScopes, debugScopes.debugHead, debugScopes.timerHead);

            // Only the first raster command of a draw sequence has a valid pass index.
            // The rest are coalesced. so by that token we can end rendering based on if the passIndex is valid.
            if (command->passIndex)
            {
                ctx.cmb->EndRenderPass();
            }

            if (command->memoryBarrier || command->imageTransitionCount)
            {
                ctx.cmb->Barrier(command->memoryBarrier, command->imageTransitions, command->imageTransitionCount);
            }
            
            command->call(&ctx, command);
        }

        for (auto i = 0u; i < queueCount; ++i)
        {
            SubmitQueue(i, VK_NULL_HANDLE, &debugScopes);
        }
        
        scope.fence = queues->GetFenceRef();
        scope.timerTimelineIndex = m_timerArena->EndTimeline();
        scope.invocationIndex = m_invocationCounter;
        BeginGraph();
    }


    void VulkanGraph::BeginGraph()
    {
        m_invocationCounter++;
        auto& scope = m_scopes[m_invocationCounter % PK_VK_MAX_GRAPH_SCOPES];

        PK_FATAL_ASSERT(scope.fence.Wait(UINT64_MAX), "Failed to acquire new graph scope!");

        for (auto i = 0u; i < PK_VK_MAX_GRAPH_SCOPES; ++i)
        {
            if (m_scopes[i].fence.IsValid() && m_scopes[i].fence.IsComplete())
            {
                m_timerArena->FlushTimeline(m_scopes[i].timerTimelineIndex);
                m_stagingArena->FreeRange(m_scopes[i].stagingRangeIndex);
                m_descriptorArena->FreeRange(m_scopes[i].descriptorRangeIndex);
                m_scopes[i].fence.Invalidate();
            }
        }

        m_pipelineState = PipelineState();
        m_stagingArena->BeginRange();
        m_descriptorArena->BeginRange();
        m_accessRecorder.ResetPasses();
        RHIGraph::Clear();

    }

    VulkanGraphScope& VulkanGraph::EndGraph()
    {
        auto& scope = m_scopes[m_invocationCounter % PK_VK_MAX_GRAPH_SCOPES];
        scope.stagingRangeIndex = m_stagingArena->EndRange();
        scope.descriptorRangeIndex = m_descriptorArena->EndRange();
        return scope;
    }


    void VulkanGraph::ApplyDebugScopes(uint32_t queueIndex, VulkanGraphDebugScopes* scopes, uint32_t debugTarget, uint32_t timerTarget)
    {
        auto commandbuffer = m_driver->queues->GetQueueAt(queueIndex)->GetCommandBuffer();

        auto& debugCount = scopes->debugCount[queueIndex];
        auto& debugMin = scopes->debugMin[queueIndex];
        auto& timerCount = scopes->timerCount[queueIndex];
        auto& timerMin = scopes->timerMin[queueIndex];
        auto* timerQueries = scopes->timerQueries[queueIndex];
        const auto minDebug = math::min(debugMin, debugTarget);
        const auto minTimer = math::min(timerMin, timerTarget);

        for (; debugCount > minDebug; --debugCount)
        {
            vkCmdEndDebugUtilsLabelEXT(*commandbuffer);
        }

        for (; debugCount < debugTarget; ++debugCount)
        {
            VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
            labelInfo.pLabelName = scopes->debugStack[debugCount]->label;
            memcpy(labelInfo.color, &scopes->debugStack[debugCount]->color.x, sizeof(PK::color));
            vkCmdBeginDebugUtilsLabelEXT(*commandbuffer, &labelInfo);
        }

        while (timerCount > minTimer)
        {
            const auto queryIndex = timerQueries[--timerCount];

            if (queryIndex != ~0u)
            {
                vkCmdWriteTimestamp2(*commandbuffer, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, m_timerArena->GetQueryPool(), queryIndex + 1u);
            }
        }

        while (timerCount < timerTarget)
        {
            const auto queryIndex = m_timerArena->AddTimer(scopes->timerStack[timerCount]->name);

            if (queryIndex != ~0u)
            {
                vkCmdWriteTimestamp2(*commandbuffer, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, m_timerArena->GetQueryPool(), queryIndex);
            }

            timerQueries[timerCount++] = queryIndex;
        }

        debugMin = debugCount;
        timerMin = timerCount;
    }

    void VulkanGraph::SubmitQueue(uint32_t queueIndex, VkSemaphore signal, VulkanGraphDebugScopes* scopes)
    {
        auto queue = m_driver->queues->GetQueueAt(queueIndex);

        if (queue->IsRecording())
        {
            ApplyDebugScopes(queueIndex, scopes, 0u, 0u);
            queue->Submit(signal);
        }
    }


    uint64_t VulkanGraph::AddBarrierPass(QueueType queue, bool isRaster)
    {
        auto queueIndex = m_driver->queues->GetQueueIndex(queue);

        if (!isRaster)
        {
            m_pipelineState.coalescingQueue = ~0u;
            return m_accessRecorder.AddPass();
        }

        if (m_pipelineState.coalescingQueue != queueIndex || (m_pipelineState.dirtyFlags & STATE_DIRTY_RENDERTARGET) != 0u)
        {
            m_pipelineState.coalescingQueue = queueIndex;
            return m_accessRecorder.AddPass();
        }
     
        return 0u;
    }

    void VulkanGraph::RecordBufferAccess(QueueType queue, const VulkanBindHandle* handle, VkPipelineStageFlags2 stage, VkAccessFlags2 access)
    {
        VulkanAccessRecord record;
        record.buffer.offset = handle->buffer.offset;
        record.buffer.size = handle->buffer.range;
        record.stage = stage;
        record.access = access;
        record.queue = m_driver->queues->GetQueueIndex(queue);
        m_accessRecorder.RecordBufferAccess(handle->buffer.buffer, record);
    }

    void VulkanGraph::RecordImageAccess(QueueType queue, const VulkanBindHandle* handle, VkPipelineStageFlags2 stage, VkAccessFlags2 access)
    {
        VulkanAccessRecord record;
        record.image.baseMipLevel = handle->image.range.baseMipLevel;
        record.image.levelCount = handle->image.range.levelCount;
        record.image.baseArrayLayer = handle->image.range.baseArrayLayer;
        record.image.layerCount = handle->image.range.layerCount;
        record.stage = stage;
        record.access = access;
        record.queue = m_driver->queues->GetQueueIndex(queue);
        m_accessRecorder.RecordImageAccess(handle->image.image, record);
    
        // Track alias as well so that layout stays consistent
        if (handle->image.alias)
        {
            m_accessRecorder.RecordImageAccess(handle->image.alias, record);
        }
    }


    void VulkanGraph::RecordShaderCmd(QueueType queue, VulkanShaderCmd* cmd)
    {
        auto state = &m_pipelineState;
        const auto resources = &m_globalResources;
        const auto shader = state->pipelineKey.shader;

        PK_DEBUG_FATAL_ASSERT(shader != nullptr, "Pipeline validation failed! Shader is unassigned!");

        const auto* descriptorLayout = shader->GetDescriptorSetLayout();
        const auto& resourceLayout = shader->GetResourceLayout();
        const auto layoutStageFlags = VulkanEnumConvert::GetPipelineStageFlags(descriptorLayout->stageFlags);

        for (auto index = 0u; index < resourceLayout.GetCount(); ++index)
        {
            const auto& element = resourceLayout[index];
            const auto access = element.writeMask != 0u ? VK_ACCESS_SHADER_WRITE_BIT : VK_ACCESS_NONE;
            const auto isVariableSize = element.count == PK_RHI_MAX_UNBOUNDED_SIZE;

            auto& descriptor = state->descriptors[index];
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
                version = (uint32_t)handles[0]->Version();
                count = element.count;
            }

            // Hash fixed size array version.
            for (auto i = 1u; i < count && !isVariableSize; ++i)
            {
                auto hash = (uint32_t)handles[i]->Version();
                hash += 0x9e3779b9u + (version << 6u) + (version >> 2u);
                version ^= hash;
            }

            if (descriptor.handles != handles || descriptor.count != count || descriptor.type != element.type || descriptor.version != version)
            {
                state->dirtyFlags |= STATE_DIRTY_DESCRIPTORS;
                descriptor.handles = handles;
                descriptor.count = (uint16_t)count;
                descriptor.type = element.type;
                descriptor.version = version;
                descriptor.isVariableSize = isVariableSize;
            }

            for (auto i = 0u; i < count; ++i)
            {
                if (descriptor.type == ShaderResourceType::SamplerTexture || descriptor.type == ShaderResourceType::Texture || descriptor.type == ShaderResourceType::Image)
                {
                    RecordImageAccess(queue, handles[i], layoutStageFlags, access | VK_ACCESS_2_SHADER_READ_BIT);
                }

                if (descriptor.type == ShaderResourceType::StorageBuffer)
                {
                    RecordBufferAccess(queue, handles[i], layoutStageFlags, access | VK_ACCESS_2_SHADER_READ_BIT);
                }

                if (descriptor.type == ShaderResourceType::ConstantBuffer)
                {
                    RecordBufferAccess(queue, handles[i], layoutStageFlags, access | VK_ACCESS_2_UNIFORM_READ_BIT);
                }
            }
        }

        if (state->descriptorCount != resourceLayout.GetCount() || state->descriptorLayout != descriptorLayout)
        {
            state->dirtyFlags |= STATE_DIRTY_DESCRIPTORS;
            state->descriptorCount = (uint32_t)resourceLayout.GetCount();
            state->descriptorLayout = descriptorLayout;
        }

        if (state->dirtyFlags & STATE_DIRTY_DESCRIPTORS)
        {
            state->descriptorSetOffset = m_descriptorArena->AllocateDescriptorSet(descriptorLayout, state->descriptors, state->descriptorCount);
        }

        if (state->dirtyFlags & STATE_DIRTY_PIPELINE)
        {
            state->pipeline = m_driver->pipelineCache->GetPipeline(state->pipelineKey);
        }

        state->dirtyFlags &= ~(STATE_DIRTY_PIPELINE | STATE_DIRTY_DESCRIPTORS);

        cmd->pipeline = state->pipeline;
        cmd->pipelineLayout = shader->GetPipelineLayout();
        cmd->descriptorSetOffset = state->descriptorSetOffset;
        cmd->pushConstants = Allocate<uint8_t>(shader->GetPipelineLayout()->pushConstantSize);

        const char* data = nullptr;
        size_t dataSize = 0u;

        for (const auto& element : shader->GetPushConstantLayout())
        {
            if (resources->TryGet<char>(element.name, &data, &dataSize) && dataSize <= element.size)
            {
                memcpy(static_cast<uint8_t*>(cmd->pushConstants) + element.offset, data, dataSize);
            }
        }
    }

    void VulkanGraph::RecordRasterCmd(VulkanRasterCmd* cmd)
    {
        auto state = &m_pipelineState;
        const bool isNewPass = cmd->passIndex != 0u;
        const bool isDirtyRenderTarget = state->dirtyFlags & STATE_DIRTY_RENDERTARGET;
        const bool isDirtyViewports = state->dirtyFlags & STATE_DIRTY_VIEWPORTS;
        const bool isDirtyScissors  = state->dirtyFlags & STATE_DIRTY_SCISSORS;
        state->dirtyFlags &= ~(STATE_DIRTY_RENDERTARGET | STATE_DIRTY_VIEWPORTS | STATE_DIRTY_SCISSORS);

        if (isDirtyRenderTarget || isNewPass)
        {
            auto& colors = state->renderTarget.colors;
            auto& depth = state->renderTarget.depth;
            const auto colorCount = state->renderTarget.colorCount;
            const auto depthFormat = depth.target && depth.target->image.image ? depth.target->image.format : VK_FORMAT_UNDEFINED;

            auto colorAttachments = Allocate<VkRenderingAttachmentInfo>(state->renderTarget.colorCount);
            auto depthAttachment = Allocate<VkRenderingAttachmentInfo>(depthFormat != VK_FORMAT_UNDEFINED);

            state->renderingInfo = Allocate<VkRenderingInfo>(1u);
            state->renderingInfo->sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            state->renderingInfo->renderArea = state->renderTarget.area;
            state->renderingInfo->layerCount = state->renderTarget.layers;
            state->renderingInfo->colorAttachmentCount = state->renderTarget.colorCount;
            state->renderingInfo->pColorAttachments = colorAttachments;
            state->renderingInfo->pDepthAttachment = depthAttachment;

            for (auto i = 0u; i < PK_RHI_MAX_RENDER_TARGETS; ++i)
            {
                auto& color = colors[i];
                const auto format = i < colorCount && color.target ? color.target->image.format : VK_FORMAT_UNDEFINED;

                if (state->pipelineKey.fixed.colorFormats[i] != format)
                {
                    state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                    state->pipelineKey.fixed.colorFormats[i] = format;
                }

                if (format)
                {
                    auto& attachment = colorAttachments[i];
                    attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                    attachment.pNext = nullptr;
                    attachment.imageView = color.target->image.view;
                    attachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    attachment.resolveMode = color.resolveMode;
                    attachment.resolveImageView = color.resolve ? color.resolve->image.view : nullptr;
                    attachment.resolveImageLayout = color.resolve ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED;
                    attachment.loadOp = VulkanEnumConvert::GetLoadOp(color.loadOp);
                    attachment.storeOp = VulkanEnumConvert::GetStoreOp(color.storeOp);
                    attachment.clearValue = VulkanEnumConvert::GetClearValue(color.clearValue);
                    
                    color.loadOp = LoadOp::Load;

                    if (state->pipelineKey.fixed.sampleCountFlags != color.target->image.samples)
                    {
                        state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                        state->pipelineKey.fixed.sampleCountFlags = color.target->image.samples;
                    }

                    RecordImageAccess(QueueType::Graphics, color.target, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

                    if (color.resolve && color.resolve->image.image)
                    {
                        RecordImageAccess(QueueType::Graphics, color.resolve, VK_PIPELINE_STAGE_2_RESOLVE_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
                    }
                }
            }

            if (state->pipelineKey.fixed.depthFormat != depthFormat)
            {
                state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                state->pipelineKey.fixed.depthFormat = depthFormat;
            }

            if (depthFormat)
            {
                const auto isStencil = VulkanEnumConvert::IsDepthStencilFormat(depthFormat);
                const auto isReadOnly = !state->pipelineKey.fixed.depthStencil.depthWriteEnable && (!isStencil || state->pipelineKey.fixed.depthStencil.stencilTestEnable);

                depthAttachment->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                depthAttachment->pNext = nullptr;
                depthAttachment->imageView = depth.target->image.view;
                depthAttachment->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                depthAttachment->resolveMode = depth.resolveMode;
                depthAttachment->resolveImageView = depth.resolve ? depth.resolve->image.view : nullptr;
                depthAttachment->resolveImageLayout = depth.resolve ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment->loadOp = VulkanEnumConvert::GetLoadOp(depth.loadOp);
                depthAttachment->storeOp = VulkanEnumConvert::GetStoreOp(depth.storeOp);
                depthAttachment->clearValue = VulkanEnumConvert::GetClearValue(depth.clearValue);

                depth.loadOp = LoadOp::Load;

                if (isReadOnly)
                {
                    depthAttachment->storeOp = VK_ATTACHMENT_STORE_OP_NONE;
                }

                if (isStencil)
                {
                    state->renderingInfo->pStencilAttachment = depthAttachment;
                }

                if (state->pipelineKey.fixed.sampleCountFlags != depth.target->image.samples)
                {
                    state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                    state->pipelineKey.fixed.sampleCountFlags = depth.target->image.samples;
                }

                RecordImageAccess(QueueType::Graphics, depth.target, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | (!isReadOnly ? VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0u));

                if (depth.resolve && depth.resolve->image.image)
                {
                    RecordImageAccess(QueueType::Graphics, depth.resolve, VK_PIPELINE_STAGE_2_RESOLVE_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
                }
            }
        }

        if (isDirtyViewports)
        {
            state->resolvedViewports = Allocate<VkViewport>(state->viewportCount);
            Memory::Memcpy(state->resolvedViewports, state->viewports, state->viewportCount);
        }

        if (isDirtyScissors)
        {
            state->resolvedScissors = Allocate<VkRect2D>(state->scissorCount);
            Memory::Memcpy(state->resolvedScissors, state->scissors, state->scissorCount);
        }
        
        cmd->renderingInfo = state->renderingInfo;
        cmd->viewports = state->resolvedViewports;
        cmd->scissors = state->resolvedScissors;
        cmd->viewportCount = state->viewportCount;
        cmd->scissorCount = state->scissorCount;
        RecordShaderCmd(QueueType::Graphics, cmd);
    }

    void VulkanGraph::RecordVertexCmd(VulkanVertexCmd* cmd)
    {
        auto state = &m_pipelineState;
        const auto shader = state->pipelineKey.shader;
        const auto& vertexLayout = shader->GetVertexLayout();
        const bool isDirtyShader = state->dirtyFlags & STATE_DIRTY_SHADER;
        const bool isDirtyBuffers = state->dirtyFlags & STATE_DIRTY_VERTEXBUFFERS;
        state->dirtyFlags &= ~(STATE_DIRTY_VERTEXBUFFERS | STATE_DIRTY_SHADER);

        if (isDirtyShader)
        {
            auto index = 0u;
            for (; index < vertexLayout.GetCount(); ++index)
            {
                auto format = VulkanEnumConvert::GetFormat(vertexLayout[index].format);

                if (state->pipelineKey.vertexAttributes[index].location != vertexLayout[index].location ||
                    state->pipelineKey.vertexAttributes[index].format != format)
                {
                    state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                    state->pipelineKey.vertexAttributes[index].location = vertexLayout[index].location;
                    state->pipelineKey.vertexAttributes[index].format = format;
                }
            }

            if (index < PK_RHI_MAX_VERTEX_ATTRIBUTES && state->pipelineKey.vertexAttributes[index].format != VK_FORMAT_UNDEFINED)
            {
                state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                Memory::Memset(state->pipelineKey.vertexAttributes + index, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - index);
            }
        }

        if (isDirtyShader || isDirtyBuffers)
        {
            auto streamIndex = 0u;
            bool reallocateBuffers = isDirtyBuffers;
            auto remainingAttributes = vertexLayout.GetCount();
            const VertexStreamElement* prevElement = nullptr;
            const VulkanBindHandle* activeBuffers[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};

            for (auto index = 0u; index < PK_RHI_MAX_VERTEX_ATTRIBUTES && state->vertexStreamLayout[index].stride != 0; ++index)
            {
                const auto& element = state->vertexStreamLayout[index];
                auto elementIdx = 0u;

                if (state->vertexBuffers[element.stream] && vertexLayout.TryGetElement(element.name, &elementIdx))
                {
                    auto inputRate = VulkanEnumConvert::GetInputRate(element.inputRate);
                    --remainingAttributes;

                    if (prevElement && (prevElement->stream != element.stream || prevElement->inputRate != element.inputRate || prevElement->stride != element.stride))
                    {
                        ++streamIndex;
                    }

                    prevElement = &element;
                    activeBuffers[streamIndex] = state->vertexBuffers[element.stream];
                    reallocateBuffers |= (state->resolvedVertexHandles[streamIndex] != activeBuffers[streamIndex]);

                    if (state->pipelineKey.vertexStreams[streamIndex].binding != streamIndex ||
                        state->pipelineKey.vertexStreams[streamIndex].inputRate != inputRate ||
                        state->pipelineKey.vertexStreams[streamIndex].stride != element.stride)
                    {
                        state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                        state->pipelineKey.vertexStreams[streamIndex].binding = (uint16_t)streamIndex;
                        state->pipelineKey.vertexStreams[streamIndex].inputRate = inputRate;
                        state->pipelineKey.vertexStreams[streamIndex].stride = element.stride;
                    }

                    if (state->pipelineKey.vertexAttributes[elementIdx].binding != streamIndex ||
                        state->pipelineKey.vertexAttributes[elementIdx].offset != element.offset)
                    {
                        state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                        state->pipelineKey.vertexAttributes[elementIdx].binding = (uint16_t)streamIndex;
                        state->pipelineKey.vertexAttributes[elementIdx].offset = element.offset;
                    }
                }
            }

            PK_DEBUG_WARNING_ASSERT(remainingAttributes == 0, "Warning '%u' of shader vertex input attributes were not bound.", remainingAttributes);

            const auto streamCount = prevElement ? (streamIndex + 1u) : 0u;
            reallocateBuffers |= state->vertexBufferCount != streamCount;

            if (streamCount < PK_RHI_MAX_VERTEX_ATTRIBUTES && state->pipelineKey.vertexStreams[streamCount].stride != 0)
            {
                state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                Memory::Memset(state->pipelineKey.vertexStreams + streamCount, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - streamCount);
            }

            if (reallocateBuffers)
            {
                state->vertexBufferCount = streamCount;
                state->resolvedVertexBuffers = Allocate<VkBuffer>(streamCount);
                state->resolvedVertexBufferOffsets = Allocate<VkDeviceSize>(streamCount);

                for (auto i = 0u; i < streamCount; ++i)
                {
                    state->resolvedVertexHandles[i] = activeBuffers[i];
                    state->resolvedVertexBuffers[i] = activeBuffers[i]->buffer.buffer;
                    state->resolvedVertexBufferOffsets[i] = activeBuffers[i]->buffer.offset;
                }
            }
        }

        for (auto i = 0u; i < state->vertexBufferCount; ++i)
        {
            RecordBufferAccess(QueueType::Graphics, state->resolvedVertexHandles[i], VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT);
        }

        cmd->vertexBufferCount = state->vertexBufferCount;
        cmd->vertexBuffers = state->resolvedVertexBuffers;
        cmd->vertexBufferOffsets = state->resolvedVertexBufferOffsets;
        RecordRasterCmd(cmd);
    }

    void VulkanGraph::RecordIndexCmd(VulkanIndexCmd* cmd)
    {
        if (m_pipelineState.indexBuffer != nullptr)
        {
            RecordBufferAccess(QueueType::Graphics, m_pipelineState.indexBuffer, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_ACCESS_2_INDEX_READ_BIT);
            cmd->indexBuffer = m_pipelineState.indexBuffer->buffer.buffer;
            cmd->indexBufferOffset = m_pipelineState.indexBuffer->buffer.offset;
            cmd->indexType = m_pipelineState.indexType;
        }

        RecordVertexCmd(cmd);
    }
}
