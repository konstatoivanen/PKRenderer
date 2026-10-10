#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "Core/RHI/Vulkan/VulkanBindSet.h"
#include "Core/RHI/Vulkan/VulkanSwapchain.h"
#include "Core/RHI/Vulkan/VulkanAccelerationStructure.h"
#include "Core/RHI/Vulkan/VulkanResourceState.h"
#include "VulkanGraph.h"

namespace PK
{
    // TEMP
    RHIGraph::~RHIGraph() = default;

    static void ApplyShaderCmd(VkCommandBuffer commandBuffer, VulkanShaderCmd* cmd)
    {
        auto bufferIndex = 0u;
        if (cmd->dirtyFlags & VulkanGraph::STATE_DIRTY_PIPELINE)    vkCmdBindPipeline(commandBuffer, cmd->bindpoint, cmd->pipeline);
        if (cmd->dirtyFlags & VulkanGraph::STATE_DIRTY_DESCRIPTORS) vkCmdSetDescriptorBufferOffsetsEXT(commandBuffer, cmd->bindpoint, cmd->pipelineLayout, 0, 1, &bufferIndex, &cmd->descriptorSetOffset);
        if (cmd->dirtyFlags & VulkanGraph::STATE_DIRTY_CONSTANTS)   vkCmdPushConstants(commandBuffer, cmd->pipelineLayout, cmd->pushConstantStageFlags, 0u, cmd->pushConstantsSize, cmd->pushConstants);
    }
    
    static void ApplyRasterCmd(VkCommandBuffer commandBuffer, VulkanRasterCmd* cmd)
    {
        if (cmd->dirtyFlags & VulkanGraph::STATE_DIRTY_RENDERTARGET) vkCmdBeginRendering(commandBuffer, cmd->renderingInfo);
        if (cmd->dirtyFlags & VulkanGraph::STATE_DIRTY_VIEWPORTS)    vkCmdSetViewport(commandBuffer, 0, cmd->viewportCount, cmd->viewports);
        if (cmd->dirtyFlags & VulkanGraph::STATE_DIRTY_SCISSORS)     vkCmdSetScissor(commandBuffer, 0, cmd->scissorCount, cmd->scissors);
        ApplyShaderCmd(commandBuffer, cmd);
    }
    
    static void ApplyVertexCmd(VkCommandBuffer commandBuffer, VulkanVertexCmd* cmd)
    {
        ApplyRasterCmd(commandBuffer, cmd);
        if (cmd->dirtyFlags & VulkanGraph::STATE_DIRTY_VERTEXBUFFERS) vkCmdBindVertexBuffers(commandBuffer, 0, cmd->vertexBufferCount, cmd->vertexBuffers, cmd->vertexBufferOffsets);
    }
    
    static void ApplyIndexCmd(VkCommandBuffer commandBuffer, VulkanIndexCmd* cmd)
    {
        ApplyVertexCmd(commandBuffer, cmd);
        if (cmd->dirtyFlags & VulkanGraph::STATE_DIRTY_INDEXBUFFER) vkCmdBindIndexBuffer(commandBuffer, cmd->indexBuffer, cmd->indexBufferOffset, cmd->indexType);
    }

    VulkanGraph::VulkanGraph(const VulkanDriver* driver) : 
        m_driver(driver),
        m_resourceState(),
        m_stagingArena(driver, driver->properties.stagingSizeGraphics),
        m_descriptorArena(driver, driver->physicalDeviceProperties, driver->properties.descriptorSizeGraphics),
        m_timerArena(driver->device, driver->physicalDeviceProperties.core.limits.timestampPeriod),
        m_globalResources(PK_VK_GLOBAL_PROPERTIES_INITIAL_SIZE, PK_VK_GLOBAL_PROPERTIES_INITIAL_COUNT)
    {
        for (auto i = 0u; i < (uint32_t)QueueType::EnumCount; ++i)
        {
            m_queueFamilies[i] = driver->queues->GetQueue((QueueType)i)->GetFamily();
            m_queueIndices[i] = driver->queues->GetQueueIndex((QueueType)i);
        }

        m_queueCount = driver->queues->GetSelectedFamilies().count;
        m_stagingArena.BeginRange();
        m_descriptorArena.BeginRange();
        m_timerArena.BeginTimeline();
        m_resourceState->ResetPasses();
        RHIGraph::Clear();
    }

    VulkanGraph::~VulkanGraph()
    {
        // @TODO do we have any destruction dependencies?!?
    }

    void VulkanGraph::Execute()
    {
        m_stagingRangeIndex = m_stagingArena.EndRange();
        m_descriptorRangeIndex = m_descriptorArena.EndRange();
        m_timerTimelineIndex = m_timerArena.EndTimeline();

        VulkanTransferBatch* activeBatches[(uint32_t)QueueType::EnumCount][(uint32_t)QueueType::EnumCount] = {};
        auto maxTransfers = 0u;

        for (auto cmd = static_cast<VulkanCmd*>(GetCommands()); cmd; cmd = static_cast<VulkanCmd*>(cmd->next))
        {
            cmd->transfers = nullptr;
            cmd->imageBarriers = nullptr;
            cmd->bufferBarriers = nullptr;
            cmd->imageBarrierCount = 0u;
            cmd->bufferBarrierCount = 0u;

            const auto dstQueueIndex = GetQueueIndex(cmd->queue);

            for (auto i = 0u; i < (uint32_t)QueueType::EnumCount; ++i)
            {
                activeBatches[dstQueueIndex][i] = nullptr;
            }

            if (!cmd->passIndex)
            {
                continue;
            }

            auto imageBarrierCount = 0u;
            const VkImageMemoryBarrier2* imageBarriers = nullptr;

            m_resourceState->GetPassBarriers(
                cmd->passIndex,
                &cmd->bufferBarrierCount,
                &cmd->bufferBarriers,
                &imageBarrierCount,
                &imageBarriers);

            auto transferCount = 0u;

            for (auto i = 0u; i < imageBarrierCount; ++i)
            {
                const auto& barrier = imageBarriers[i];
                if (barrier.srcQueueFamilyIndex != barrier.dstQueueFamilyIndex)
                {
                    transferCount++;
                }
            }

            if (!transferCount)
            {
                cmd->imageBarriers = imageBarriers;
                cmd->imageBarrierCount = imageBarrierCount;
            }
            else
            {
                auto* sameQueueSlice = Allocate<VkImageMemoryBarrier2>(imageBarrierCount - transferCount);
                cmd->imageBarriers = sameQueueSlice;
                cmd->imageBarrierCount = 0u;

                for (auto i = 0u; i < imageBarrierCount; ++i)
                {
                    const auto& barrier = imageBarriers[i];

                    if (barrier.srcQueueFamilyIndex != barrier.dstQueueFamilyIndex)
                    {
                        const auto srcQueueIndex = GetQueueIndexFromFamily(barrier.srcQueueFamilyIndex);
                        auto& batch = activeBatches[srcQueueIndex][dstQueueIndex];

                        if (!batch)
                        {
                            batch = Allocate<VulkanTransferBatch>(1u);
                            batch->srcQueueIndex = srcQueueIndex;
                            batch->dstQueueIndex = dstQueueIndex;
                            cmd->transfers = batch;
                        }

                        auto* node = Allocate<VulkanImageTransfer>(1u);
                        node->barrier = barrier;
                        node->next = batch->head;
                        batch->head = node;
                        batch->count++;
                        batch->srcStageMask |= barrier.srcStageMask;
                        batch->dstStageMask |= barrier.dstStageMask;
                        maxTransfers = math::max(maxTransfers, batch->count);
                    }
                    else if (sameQueueSlice)
                    {
                        sameQueueSlice[cmd->imageBarrierCount++] = barrier;
                    }
                }
            }
        }

        auto* releaseBarriers = Allocate<VkImageMemoryBarrier2>(maxTransfers);
        auto* acquireBarriers = Allocate<VkImageMemoryBarrier2>(maxTransfers);
        uint64_t dirtyFlags[(uint32_t)QueueType::EnumCount]{};
        bool isRendering[(uint32_t)QueueType::EnumCount]{};
        bool isRecording[(uint32_t)QueueType::EnumCount]{};
        memset(dirtyFlags, 0xFFu, sizeof(dirtyFlags));

        for (auto command = static_cast<VulkanCmd*>(GetCommands()); command; command = static_cast<VulkanCmd*>(command->next))
        {
            const auto queueIndex = GetQueueIndex(command->queue);
            const auto queue = m_driver->queues->GetQueueAt(queueIndex);
            auto* transfers = command->transfers;

            if (transfers)
            {
                const auto srcIdx = transfers->srcQueueIndex;
                const auto dstIdx = transfers->dstQueueIndex;
                auto queueSrc = m_driver->queues->GetQueueAt(srcIdx);
                auto queueDst = m_driver->queues->GetQueueAt(dstIdx);

                if (isRendering[srcIdx])
                {
                    vkCmdEndRendering(queueSrc->GetCommandBuffer()->GetHandle());
                    isRendering[srcIdx] = false;
                }

                if (isRendering[dstIdx])
                {
                    vkCmdEndRendering(queueDst->GetCommandBuffer()->GetHandle());
                    isRendering[dstIdx] = false;
                }

                // flush independent stuff before sync.
                if (isRecording[dstIdx])
                {
                    queueDst->Submit();
                    dirtyFlags[dstIdx] = ~0ull;
                    isRecording[dstIdx] = false;
                }

                auto transferIndex = 0u;
                for (auto* curr = transfers->head; curr; curr = curr->next)
                {
                    releaseBarriers[transferIndex] = curr->barrier;
                    releaseBarriers[transferIndex].dstStageMask = VK_PIPELINE_STAGE_2_NONE;
                    releaseBarriers[transferIndex].dstAccessMask = VK_ACCESS_2_NONE;
                    acquireBarriers[transferIndex] = curr->barrier;
                    acquireBarriers[transferIndex].srcStageMask = VK_PIPELINE_STAGE_2_NONE;
                    acquireBarriers[transferIndex++].srcAccessMask = VK_ACCESS_2_NONE;
                }

                VkDependencyInfo releaseBarrier{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
                releaseBarrier.imageMemoryBarrierCount = transfers->count;
                releaseBarrier.pImageMemoryBarriers = releaseBarriers;
                vkCmdPipelineBarrier2(queueSrc->GetCommandBuffer()->GetHandle(), &releaseBarrier);

                queueSrc->Submit();
                queueDst->QueueWait(queueSrc, transfers->dstStageMask, 0);

                dirtyFlags[srcIdx] = ~0ull;
                isRecording[srcIdx] = false;

                VkDependencyInfo acquireBarrier{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
                acquireBarrier.imageMemoryBarrierCount = transfers->count;
                acquireBarrier.pImageMemoryBarriers = acquireBarriers;
                vkCmdPipelineBarrier2(queueDst->GetCommandBuffer()->GetHandle(), &acquireBarrier);
            }

            command->dirtyFlags |= dirtyFlags[queueIndex];
            dirtyFlags[queueIndex] &= ~command->dirtyFlagMask;
            isRecording[queueIndex] = true;

            const auto hasBarriers = command->bufferBarrierCount || command->imageBarrierCount;
            const auto isRasterCmd = (command->dirtyFlagMask & STATE_DIRTY_RENDERTARGET) != 0;
            const auto isRasterBeg = (command->dirtyFlags & STATE_DIRTY_RENDERTARGET) != 0;

            if (isRendering[queueIndex])
            {
                if (hasBarriers || !isRasterCmd || isRasterBeg)
                {
                    vkCmdEndRendering(queue->GetCommandBuffer()->GetHandle());
                    isRendering[queueIndex] = false;
                }
            }

            if (hasBarriers)
            {
                VkDependencyInfo barrier{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
                barrier.bufferMemoryBarrierCount = command->bufferBarrierCount;
                barrier.pBufferMemoryBarriers = command->bufferBarriers;
                barrier.imageMemoryBarrierCount = command->imageBarrierCount;
                barrier.pImageMemoryBarriers = command->imageBarriers;
                vkCmdPipelineBarrier2(queue->GetCommandBuffer()->GetHandle(), &barrier);
            }
            
            VulkanCmdContext ctx{};
            ctx.cmb = queue->GetCommandBuffer()->GetHandle();
            ctx.swapchain = nullptr;
            ctx.acquireSignal = VK_NULL_HANDLE;
            ctx.presentSignal = VK_NULL_HANDLE;
            command->call(&ctx, command);

            if (isRasterBeg)
            {
                isRendering[queueIndex] = true;
            }

            if (ctx.swapchain)
            {
                if (ctx.acquireSignal != VK_NULL_HANDLE)
                {
                    queue->QueueWait(ctx.acquireSignal, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
                }

                if (ctx.presentSignal != VK_NULL_HANDLE)
                {
                    if (isRendering[queueIndex])
                    {
                        vkCmdEndRendering(ctx.cmb);
                        isRendering[queueIndex] = false;
                    }
                    
                    queue->Submit(&ctx.presentSignal);
                    ctx.swapchain->GraphPresent(ctx.presentSignal);
                    dirtyFlags[queueIndex] = ~0ull;
                    isRecording[queueIndex] = false;
                }
            }
        }

        for (auto i = 0u; i < (uint32_t)QueueType::EnumCount; ++i)
        {
            if (isRecording[i])
            {
                auto queue = m_driver->queues->GetQueueAt(i);

                if (isRendering[i])
                {
                    vkCmdEndRendering(queue->GetCommandBuffer()->GetHandle());
                }

                queue->Submit();
            }
        }
        
        m_stagingArena.BeginRange();
        m_descriptorArena.BeginRange();
        m_timerArena.BeginTimeline();
        m_resourceState->ResetPasses();
        RHIGraph::Clear();
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

    RHIBuffer* VulkanGraph::AcquireStagingBuffer(size_t size) { return m_stagingArena.BeginWrite(size); }
    void VulkanGraph::ReleaseStagingBuffer(RHIBuffer* buffer) { m_stagingArena.EndWrite(buffer); }


    void VulkanGraph::SetViewPorts(QueueType queue, const uint4* rects, uint32_t count)
    {
        auto state = GetQueueState(queue);

        for (auto i = 0u; i < count && i < PK_RHI_MAX_VIEWPORTS; ++i)
        {
            auto& rect = rects[i];
            VkViewport v = { (float)rect.x, (float)rect.y, (float)rect.z, (float)rect.w, 0.0f, 1.0f };

            if (memcmp(&state->viewports[i], &v, sizeof(VkViewport)) != 0)
            {
                state->viewports[i] = v;
                state->dirtyFlags |= STATE_DIRTY_VIEWPORTS;
            }
        }

        state->viewportCount = count;
    }

    void VulkanGraph::SetScissors(QueueType queue, const uint4* rects, uint32_t count)
    {
        auto state = GetQueueState(queue);

        if (memcmp(state->scissors, rects, sizeof(VkRect2D) * (count > PK_RHI_MAX_VIEWPORTS ? PK_RHI_MAX_VIEWPORTS : count)) != 0)
        {
            memcpy(state->scissors, rects, sizeof(VkRect2D) * (count > PK_RHI_MAX_VIEWPORTS ? PK_RHI_MAX_VIEWPORTS : count));
            state->dirtyFlags |= STATE_DIRTY_SCISSORS;
            state->scissorCount = count;
        }
    }

    void VulkanGraph::SetRenderTarget(QueueType queue, const RenderTargetBinding* bindings, uint32_t count, const uint4& renderArea, uint32_t layers)
    {
        auto state = GetQueueState(queue);

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

        if (memcmp(&state->renderTarget, &renderTarget, sizeof(VulkanRenderTarget)) != 0)
        {
            state->dirtyFlags |= STATE_DIRTY_RENDERTARGET;
            state->renderTarget = renderTarget;
        }
    }

    void VulkanGraph::SetStageExcludeMask(QueueType queue, const ShaderStageFlags mask)
    {
        auto state = GetQueueState(queue);

        if (state->pipelineKey.fixed.excludeStageMask != (uint16_t)mask)
        {
            state->dirtyFlags |= STATE_DIRTY_PIPELINE;
            state->pipelineKey.fixed.excludeStageMask = (uint16_t)mask;
        }
    }

    void VulkanGraph::SetBlending(QueueType queue, const BlendParameters& blend)
    {
        auto state = GetQueueState(queue);

        if (memcmp(&state->pipelineKey.fixed.blending, &blend, sizeof(BlendParameters)) != 0)
        {
            state->dirtyFlags |= STATE_DIRTY_PIPELINE;
            state->pipelineKey.fixed.blending = blend;
        }
    }

    void VulkanGraph::SetRasterization(QueueType queue, const RasterizationParameters& rasterization)
    {
        auto state = GetQueueState(queue);

        if (memcmp(&state->pipelineKey.fixed.rasterization, &rasterization, sizeof(RasterizationParameters)) != 0)
        {
            state->dirtyFlags |= STATE_DIRTY_PIPELINE;
            state->pipelineKey.fixed.rasterization = rasterization;
        }
    }

    void VulkanGraph::SetDepthStencil(QueueType queue, const DepthStencilParameters& depthStencil)
    {
        auto state = GetQueueState(queue);

        if (memcmp(&state->pipelineKey.fixed.depthStencil, &depthStencil, sizeof(DepthStencilParameters)) != 0)
        {
            state->dirtyFlags |= STATE_DIRTY_PIPELINE;
            state->pipelineKey.fixed.depthStencil = depthStencil;
        }
    }

    void VulkanGraph::SetMultisampling(QueueType queue, const MultisamplingParameters& multisampling)
    {
        auto state = GetQueueState(queue);

        if (memcmp(&state->pipelineKey.fixed.multisampling, &multisampling, sizeof(MultisamplingParameters)) != 0)
        {
            state->dirtyFlags |= STATE_DIRTY_PIPELINE;
            state->pipelineKey.fixed.multisampling = multisampling;
        }
    }

    void VulkanGraph::SetShader(QueueType queue, const RHIShader* shader)
    {
        auto state = GetQueueState(queue);

        if (state->pipelineKey.shader != shader)
        {
            state->dirtyFlags |= STATE_DIRTY_PIPELINE | STATE_DIRTY_SHADER;
            state->pipelineKey.shader = static_cast<const VulkanShader*>(shader);
        }
    }

    void VulkanGraph::SetVertexBuffers(QueueType queue, const RHIBuffer** buffers, uint32_t count)
    {
        auto state = GetQueueState(queue);

        auto i = 0u;

        for (; i < count && i < PK_RHI_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            auto handle = buffers[i]->GetNativeView<VulkanBindHandle>();
            PK_DEBUG_FATAL_ASSERT(handle != nullptr, "Passing null vertex buffer is not allowed!");

            if (handle != state->vertexBuffers[i])
            {
                state->dirtyFlags |= STATE_DIRTY_VERTEXBUFFERS;
                state->vertexBuffers[i] = handle;
            }
        }

        if (i < PK_RHI_MAX_VERTEX_ATTRIBUTES)
        {
            Memory::Memset(state->vertexBuffers + i, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - i);
        }
    }

    void VulkanGraph::SetVertexStreams(QueueType queue, const VertexStreamElement* elements, uint32_t count)
    {
        PK_DEBUG_FATAL_ASSERT(count <= PK_RHI_MAX_VERTEX_ATTRIBUTES, "Tried to bind more vertex attributes than currently supported!");

        auto state = GetQueueState(queue);

        if (memcmp(state->vertexStreamLayout, elements, sizeof(VertexStreamElement) * count) != 0)
        {
            state->dirtyFlags |= STATE_DIRTY_VERTEXBUFFERS;
            memcpy(state->vertexStreamLayout, elements, sizeof(VertexStreamElement) * count);
        }

        if (count < PK_RHI_MAX_VERTEX_ATTRIBUTES && state->vertexStreamLayout[count].stride != 0)
        {
            Memory::Memset(state->vertexStreamLayout + count, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - count);
        }
    }

    void VulkanGraph::SetIndexBuffer(QueueType queue, const RHIBuffer* buffer, size_t indexSize)
    {
        auto state = GetQueueState(queue);
        auto handle = buffer->GetNativeView<VulkanBindHandle>();
        auto indexType = VulkanEnumConvert::GetIndexType(indexSize);

        if (state->indexBuffer != handle || (handle != nullptr && state->indexType != indexType))
        {
            state->dirtyFlags |= STATE_DIRTY_INDEXBUFFER;
            state->indexBuffer = handle;
            state->indexType = indexType;
        }
    }

    void VulkanGraph::SetShaderBindingTable(QueueType queue, RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset, size_t stride, size_t size)
    {
        auto state = GetQueueState(queue);
        const auto address = buffer->GetDeviceAddress();
        state->sbtAddresses[(uint32_t)group] = { address + offset, stride, size };
    }


    void VulkanGraph::Draw(QueueType queue, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        auto cmd = AllocateCommand(queue,[](VulkanCmdContext* ctx, VulkanDrawCmd* cmd)
        {
            ApplyVertexCmd(ctx->cmb, cmd);
            vkCmdDraw(ctx->cmb, cmd->vertexCount, cmd->instanceCount, cmd->firstVertex, cmd->firstInstance);
        });
            
        cmd->passIndex = AddBarrierPass(queue, true);
        cmd->vertexCount = vertexCount;
        cmd->instanceCount = instanceCount;
        cmd->firstVertex = firstVertex;
        cmd->firstInstance = firstInstance;
        RecordVertexCmd(queue, cmd);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
    }

    void VulkanGraph::DrawIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto cmd = AllocateCommand(queue,[](VulkanCmdContext* ctx, VulkanDrawIndirectCmd* cmd)
        {
            ApplyVertexCmd(ctx->cmb, cmd);
            vkCmdDrawIndirect(ctx->cmb, cmd->indirectArguments, cmd->indirectArgumentsOffset, cmd->drawCount, cmd->stride);
        });

        auto indirectArgumentsHandle = indirectArguments->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, true);
        cmd->indirectArguments = indirectArgumentsHandle->buffer.buffer;
        cmd->indirectArgumentsOffset = offset + indirectArgumentsHandle->buffer.offset;
        cmd->drawCount = drawCount;
        cmd->stride = stride;
        RecordVertexCmd(queue, cmd);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
        RecordBufferAccess(indirectArgumentsHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::DrawIndexed(QueueType queue, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        auto cmd = AllocateCommand(queue,[](VulkanCmdContext* ctx, VulkanDrawIndexedCmd* cmd)
        {
            ApplyIndexCmd(ctx->cmb, cmd);
            vkCmdDrawIndexed(ctx->cmb, cmd->indexCount, cmd->instanceCount, cmd->firstIndex, cmd->vertexOffset, cmd->firstInstance);
        });

        cmd->passIndex = AddBarrierPass(queue, true);
        cmd->indexCount = indexCount;
        cmd->instanceCount = instanceCount;
        cmd->firstIndex = firstIndex;
        cmd->vertexOffset = vertexOffset;
        cmd->firstInstance = firstInstance;
        RecordIndexCmd(queue, cmd);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
    }

    void VulkanGraph::DrawIndexedIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto cmd = AllocateCommand(queue,[](VulkanCmdContext* ctx, VulkanDrawIndexedIndirectCmd* cmd)
        {
            ApplyIndexCmd(ctx->cmb, cmd);
            vkCmdDrawIndexedIndirect(ctx->cmb, cmd->indirectArguments, cmd->indirectArgumentsOffset, cmd->drawCount, cmd->stride);
        });

        auto indirectArgumentsHandle = indirectArguments->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, true);
        cmd->indirectArguments = indirectArgumentsHandle->buffer.buffer;
        cmd->indirectArgumentsOffset = offset + indirectArgumentsHandle->buffer.offset;
        cmd->drawCount = drawCount;
        cmd->stride = stride;
        RecordIndexCmd(queue, cmd);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
        RecordBufferAccess(indirectArgumentsHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::DrawMeshTasks(QueueType queue, const uint3& dimensions)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanDrawMeshTasksCmd* cmd)
        {
            ApplyRasterCmd(ctx->cmb, cmd);
            vkCmdDrawMeshTasksEXT(ctx->cmb, cmd->dimensions.x, cmd->dimensions.y, cmd->dimensions.z);
        });

        cmd->passIndex = AddBarrierPass(queue, true);
        cmd->dimensions = dimensions;
        RecordRasterCmd(queue, cmd);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
    }

    void VulkanGraph::DrawMeshTasksIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanDrawMeshTasksIndirectCmd* cmd)
        {
            ApplyRasterCmd(ctx->cmb, cmd);
            vkCmdDrawMeshTasksIndirectEXT(ctx->cmb, cmd->indirectArguments, cmd->indirectArgumentsOffset, cmd->drawCount, cmd->stride);
        });

        auto indirectArgumentsHandle = indirectArguments->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, true);
        cmd->indirectArguments = indirectArgumentsHandle->buffer.buffer;
        cmd->indirectArgumentsOffset = offset + indirectArgumentsHandle->buffer.offset;
        cmd->drawCount = drawCount;
        cmd->stride = stride;
        RecordRasterCmd(queue, cmd);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
        RecordBufferAccess(indirectArgumentsHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::DrawMeshTasksIndirectCount(QueueType queue, const RHIBuffer* indirectArguments, size_t offset, const RHIBuffer* countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanDrawMeshTasksIndirectCountCmd* cmd)
        {
            ApplyRasterCmd(ctx->cmb, cmd);
            vkCmdDrawMeshTasksIndirectCountEXT(ctx->cmb, cmd->indirectArguments, cmd->indirectArgumentsOffset, cmd->countBuffer, cmd->countBufferOffset, cmd->maxDrawCount, cmd->stride);
        });

        auto indirectArgumentsHandle = indirectArguments->GetNativeView<VulkanBindHandle>();
        auto countBufferHandle = countBuffer->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, true);
        cmd->indirectArguments = indirectArgumentsHandle->buffer.buffer;
        cmd->indirectArgumentsOffset = offset + indirectArgumentsHandle->buffer.offset;
        cmd->countBuffer = countBufferHandle->buffer.buffer;
        cmd->countBufferOffset = countOffset + countBufferHandle->buffer.offset;
        cmd->maxDrawCount = maxDrawCount;
        cmd->stride = stride;
        RecordRasterCmd(queue, cmd);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
        RecordBufferAccess(indirectArgumentsHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
        RecordBufferAccess(countBufferHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::Dispatch(QueueType queue, const uint3& dimensions)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanDispatchCmd* cmd)
        {
            ApplyShaderCmd(ctx->cmb, cmd);
            vkCmdDispatch(ctx->cmb, cmd->groupCounts.x, cmd->groupCounts.y, cmd->groupCounts.z);
        });

        const auto groupSize = GetQueueState(queue)->pipelineKey.shader->GetGroupSize();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->groupCounts.x = (dimensions.x + groupSize.x - 1u) / groupSize.x;
        cmd->groupCounts.y = (dimensions.y + groupSize.y - 1u) / groupSize.y;
        cmd->groupCounts.z = (dimensions.z + groupSize.z - 1u) / groupSize.z;
        RecordShaderCmd(queue, cmd);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    }

    void VulkanGraph::DispatchIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanDispatchIndirectCmd* cmd)
        {
            ApplyShaderCmd(ctx->cmb, cmd);
            vkCmdDispatchIndirect(ctx->cmb, cmd->indirectArguments, cmd->indirectArgumentsOffset);
        });

        auto indirectArgumentsHandle = indirectArguments->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->indirectArguments = indirectArgumentsHandle->buffer.buffer;
        cmd->indirectArgumentsOffset = offset + indirectArgumentsHandle->buffer.offset;
        RecordShaderCmd(queue, cmd);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        RecordBufferAccess(indirectArgumentsHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::DispatchRays(QueueType queue, const uint3& dimensions)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanDispatchRaysCmd* cmd)
        {
            ApplyShaderCmd(ctx->cmb, cmd);
            vkCmdTraceRaysKHR(ctx->cmb, &cmd->addressRayGen, &cmd->addressMiss, &cmd->addressHit, &cmd->addressCallable, cmd->dimensions.x, cmd->dimensions.y, cmd->dimensions.z);
        });

        auto state = GetQueueState(queue);
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->addressRayGen = state->sbtAddresses[(uint32_t)RayTracingShaderGroup::RayGeneration];
        cmd->addressMiss = state->sbtAddresses[(uint32_t)RayTracingShaderGroup::Miss];
        cmd->addressHit = state->sbtAddresses[(uint32_t)RayTracingShaderGroup::Hit];
        cmd->addressCallable = state->sbtAddresses[(uint32_t)RayTracingShaderGroup::Callable];
        cmd->dimensions = dimensions;
        RecordShaderCmd(queue, cmd);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR);
    }

    void VulkanGraph::Blit(QueueType queue, RHITexture* src, RHISwapchain* dst, FilterMode filter)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanBlitCmd* cmd)
        {
            auto handle = cmd->dstSwapchain->GetBindHandle();
            auto signal = cmd->dstSwapchain->ConsumeImageSignal();

            if (signal != VK_NULL_HANDLE)
            {
                ctx->acquireSignal = signal;
                ctx->swapchain = cmd->dstSwapchain;
            }

            VkImageMemoryBarrier2 imageBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            imageBarrier.srcAccessMask = 0u;
            imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.image = handle->image.image;
            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarrier.subresourceRange.baseArrayLayer = 0u;
            imageBarrier.subresourceRange.baseMipLevel = 0u;
            imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            VkDependencyInfo barrier{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
            barrier.imageMemoryBarrierCount = 1u;
            barrier.pImageMemoryBarriers = &imageBarrier;

            cmd->dst = handle->image.image;
            cmd->region.dstSubresource = { (uint32_t)handle->image.range.aspectMask, 0u, 0u, 1u };

            vkCmdPipelineBarrier2(ctx->cmb, &barrier);
            vkCmdBlitImage(ctx->cmb, cmd->src, VK_IMAGE_LAYOUT_GENERAL, handle->image.image, VK_IMAGE_LAYOUT_GENERAL, 1, &cmd->region, cmd->filter);
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

        RecordImageAccess(queue, handle, true, VK_PIPELINE_STAGE_2_BLIT_BIT_KHR, VK_ACCESS_2_TRANSFER_READ_BIT);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_BLIT_BIT_KHR);
    }

    void VulkanGraph::Blit(QueueType queue, RHISwapchain* src, RHIBuffer* dst)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanBlitSwapchainToBufferCmd* cmd)
        {
            auto handle = cmd->src->GetBindHandle();
            auto signal = cmd->src->ConsumeImageSignal();

            if (signal != VK_NULL_HANDLE)
            {
                ctx->acquireSignal = signal;
                ctx->swapchain = cmd->src;
            }

            VkImageMemoryBarrier2 imageBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            imageBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            imageBarrier.oldLayout = signal ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_GENERAL;
            imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.image = handle->image.image;
            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarrier.subresourceRange.baseArrayLayer = 0u;
            imageBarrier.subresourceRange.baseMipLevel = 0u;
            imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            VkDependencyInfo barrier{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
            barrier.imageMemoryBarrierCount = 1u;   
            barrier.pImageMemoryBarriers = &imageBarrier;

            cmd->region.imageOffset = { 0,0,0 };
            cmd->region.imageExtent = handle->image.extent;
            cmd->region.imageSubresource.aspectMask = handle->image.range.aspectMask;
            cmd->region.imageSubresource.mipLevel = 0u;
            cmd->region.imageSubresource.baseArrayLayer = 0u;
            cmd->region.imageSubresource.layerCount = VK_REMAINING_ARRAY_LAYERS;

            vkCmdPipelineBarrier2(ctx->cmb, &barrier);
            vkCmdCopyImageToBuffer(ctx->cmb, handle->image.image, VK_IMAGE_LAYOUT_GENERAL, cmd->dst, 1, &cmd->region);
        });

        auto handle = dst->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->src = static_cast<VulkanSwapchain*>(src);
        cmd->dst = handle->buffer.buffer;
        cmd->region.bufferOffset = handle->buffer.offset;
        RecordBufferAccess(handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    }

    void VulkanGraph::Blit(QueueType queue, RHITexture* src, RHITexture* dst, const TextureViewRange& srcRange, const TextureViewRange& dstRange, FilterMode filter)
    {
        BeginDebugScope(queue, "Blit Image", PK_COLOR_RED);

        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanBlitCmd* cmd)
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
                vkCmdResolveImage(ctx->cmb, cmd->src, VK_IMAGE_LAYOUT_GENERAL, cmd->dst, VK_IMAGE_LAYOUT_GENERAL, 1, &resolveRegion);
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
                vkCmdCopyImage(ctx->cmb, cmd->src, VK_IMAGE_LAYOUT_GENERAL, cmd->dst, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
            }
            else
            {
                vkCmdBlitImage(ctx->cmb, cmd->src, VK_IMAGE_LAYOUT_GENERAL, cmd->dst, VK_IMAGE_LAYOUT_GENERAL, 1, &cmd->region, cmd->filter);
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
        
        RecordImageAccess(queue, srcHandle, true, VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
        RecordImageAccess(queue, dstHandle, false, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
        RecordImageAccess(queue, dstHandle, true, VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT);
        EndDebugScope(queue);
    }

    void VulkanGraph::Clear(QueueType queue, RHIBuffer* dst, size_t offset, size_t size, uint32_t value)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanFillBufferCmd* cmd)
        {
            vkCmdFillBuffer(ctx->cmb, cmd->buffer, cmd->offset, cmd->size, cmd->value);
        });

        auto handle = dst->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->buffer = handle->buffer.buffer;
        cmd->offset = offset + handle->buffer.offset;
        cmd->size = size;
        cmd->value = value;
        RecordBufferAccess(handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    }

    void VulkanGraph::Clear(QueueType queue, RHITexture* dst, const TextureViewRange& range, const TextureClearValue& value)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanClearImageCmd* cmd)
        {
            if (cmd->range.aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
            {
                vkCmdClearDepthStencilImage(ctx->cmb, cmd->image, VK_IMAGE_LAYOUT_GENERAL, &cmd->clearValue.depthStencil, 1, &cmd->range);
            }
            else
            {
                vkCmdClearColorImage(ctx->cmb, cmd->image, VK_IMAGE_LAYOUT_GENERAL, &cmd->clearValue.color, 1, &cmd->range);
            }
        });

        auto handle = dst->GetNativeView<VulkanBindHandle>(range, TextureViewMode::UAV);
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->image = handle->image.image;
        cmd->range = handle->image.range;
        cmd->clearValue = VulkanEnumConvert::GetClearValue(value);
        RecordBufferAccess(handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    }

    void VulkanGraph::UpdateBuffer(QueueType queue, RHIBuffer* dst, size_t offset, size_t size, const void* data)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanUpdateBufferCmd* cmd)
        {
            vkCmdUpdateBuffer(ctx->cmb, cmd->buffer, cmd->offset, cmd->size, cmd->pData);
        });

        auto handle = dst->GetNativeView<VulkanBindHandle>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->buffer = handle->buffer.buffer;
        cmd->offset = offset + handle->buffer.offset;
        cmd->size = size;
        cmd->pData = Allocate<uint8_t>(size);
        memcpy(cmd->pData, data, size);
        RecordBufferAccess(handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    }

    void VulkanGraph::CopyBuffer(QueueType queue, RHIBuffer* dst, RHIBuffer* src, size_t srcOffset, size_t dstOffset, size_t size)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanCopyBufferCmd* cmd)
        {
            vkCmdCopyBuffer(ctx->cmb, cmd->src, cmd->dst, 1, &cmd->region);
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
        RecordBufferAccess(dstHandle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    }

    void VulkanGraph::CopyToTexture(QueueType queue, RHITexture* texture, RHIBuffer* buffer, TextureDataRegion* regions, uint32_t regionCount)
    {
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanCopyBufferToImageCmd* cmd)
        {
            vkCmdCopyBufferToImage(ctx->cmb, cmd->src, cmd->dst, VK_IMAGE_LAYOUT_GENERAL, cmd->regionCount, cmd->regions);
        });

        auto resourceRange = TextureViewRange();

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
        RecordImageAccess(queue, dstHandle, false, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0);
        RecordImageAccess(queue, dstHandle, true, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    }

    void VulkanGraph::InvalidateTexture(QueueType queue, RHITexture* texture)
    {
        auto handle = texture->GetNativeView<VulkanBindHandle>({}, TextureViewMode::RAW);
        RecordImageAccess(queue, handle, false, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0);
        RecordImageAccess(queue, handle, true, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0);
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
        auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanWriteAccelerationStructureCmd* cmd)
        {
            for (auto i = 0u; i < cmd->copyCount; ++i)
            {
                vkCmdCopyAccelerationStructureKHR(ctx->cmb, &cmd->copyInfos[i]);
            }

            if (cmd->BLASBuildCount)
            {
                vkCmdBuildAccelerationStructuresKHR(ctx->cmb, cmd->BLASBuildCount, cmd->BLASBuildInfos, cmd->BLASRangeInfos);
            }

            if (cmd->BLASBuildCount || cmd->copyCount)
            {
                VkMemoryBarrier2 memoryBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER_2 };
                memoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
                memoryBarrier.srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
                memoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
                memoryBarrier.dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
                VkDependencyInfo barrier{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
                barrier.memoryBarrierCount = 1u;
                barrier.pMemoryBarriers = &memoryBarrier;
                vkCmdPipelineBarrier2(ctx->cmb, &barrier);
            }

            if (cmd->queryCount)
            {
                vkCmdWriteAccelerationStructuresPropertiesKHR(ctx->cmb, cmd->queryCount, cmd->queryHandles, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, cmd->queryPool, cmd->queryStart);
            }

            if (cmd->TLASBuild)
            {
                const auto* pRangeInfo = &cmd->TLASRangeInfo;
                vkCmdBuildAccelerationStructuresKHR(ctx->cmb, 1u, &cmd->TLASBuildInfo, &pRangeInfo);
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
        cmd->queryHandles = m_driver->arena.Allocate<VkAccelerationStructureKHR>(substructureCount);
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
        RecordFlags(queue, cmd, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR);
    }

    void VulkanGraph::AcquireNextImage(RHISwapchain* swapchain)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanCmdContext* ctx, VulkanAcquireNextImageCmd* cmd)
        {
            cmd->swapchain->GraphAcquireNextImage();
            ctx->swapchain = cmd->swapchain;
        });

        cmd->swapchain = static_cast<VulkanSwapchain*>(swapchain);
        cmd->swapchain->GraphValidate();
    }

    void VulkanGraph::Present(RHISwapchain* swapchain)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanCmdContext* ctx, VulkanPresentImageCmd* cmd)
        {
            auto handle = cmd->swapchain->GetBindHandle();
            auto signal = cmd->swapchain->ConsumeImageSignal();
            ctx->acquireSignal = signal;
            ctx->presentSignal = cmd->swapchain->GraphGetPresentSignal();
            ctx->swapchain = cmd->swapchain;

            VkImageMemoryBarrier2 imageBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            imageBarrier.srcAccessMask = 0u;
            imageBarrier.dstStageMask = 0u;
            imageBarrier.dstAccessMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            imageBarrier.oldLayout = signal ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_GENERAL;
            imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.image = handle->image.image;
            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarrier.subresourceRange.baseArrayLayer = 0u;
            imageBarrier.subresourceRange.baseMipLevel = 0u;
            imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            VkDependencyInfo barrier{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
            barrier.imageMemoryBarrierCount = 1u;
            barrier.pImageMemoryBarriers = &imageBarrier;
            vkCmdPipelineBarrier2(ctx->cmb, &barrier);
        });

        cmd->swapchain = static_cast<VulkanSwapchain*>(swapchain);
    }


    void VulkanGraph::BeginDebugScope(QueueType queue, const char* name, const color& color)
    {
        if (vkCmdBeginDebugUtilsLabelEXT)
        {
            auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanBeginDebugScopeCmd* cmd)
            {
                VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
                labelInfo.pLabelName = cmd->label;
                memcpy(labelInfo.color, &cmd->color.x, sizeof(PK::color));
                vkCmdBeginDebugUtilsLabelEXT(ctx->cmb, &labelInfo);
            });

            auto length = strlen(name);
            cmd->color = color;
            cmd->label = Allocate<char>(length + 1u);
            strncpy(cmd->label, name, length);
        }
    }

    void VulkanGraph::EndDebugScope(QueueType queue)
    {
        if (vkCmdEndDebugUtilsLabelEXT)
        {
            AllocateCommand(queue, [](VulkanCmdContext* ctx, [[maybe_unused]] VulkanCmd* cmd)
            {
                vkCmdEndDebugUtilsLabelEXT(ctx->cmb);
            });
        }
    }

    void VulkanGraph::BeginTimer(QueueType queue, NameID name)
    {
        uint32_t queryIndex;

        if (m_timerArena.Push(name, &queryIndex))
        {
            auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanTimerCmd* cmd)
            {
                vkCmdWriteTimestamp(ctx->cmb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, cmd->queryPool, cmd->queryIndex);
            });

            cmd->queryPool = m_timerArena.GetQueryPool();
            cmd->queryIndex = queryIndex;
        }
    }

    void VulkanGraph::EndTimer(QueueType queue)
    {
        uint32_t queryIndex;

        if (m_timerArena.Pop(&queryIndex))
        {
            auto cmd = AllocateCommand(queue, [](VulkanCmdContext* ctx, VulkanTimerCmd* cmd)
            {
                vkCmdWriteTimestamp(ctx->cmb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, cmd->queryPool, cmd->queryIndex);
            });

            cmd->queryPool = m_timerArena.GetQueryPool();
            cmd->queryIndex = queryIndex;
        }
    }

    void VulkanGraph::AddDispatchHint(QueueType queue)
    {
        PK_FATAL_ERROR("NOT IMPLEMENTED!");
    }


    uint32_t VulkanGraph::GetQueueIndexFromFamily(uint32_t familyIndex) const
    {
        for (auto i = 0u; i < (uint32_t)QueueType::EnumCount; ++i)
        {
            if (m_queueFamilies[i] == familyIndex)
            {
                return m_queueIndices[i];
            }
        }

        return 0u;
    }

    uint64_t VulkanGraph::AddBarrierPass(QueueType queue, bool isRaster)
    {
        return m_resourceState->AddPass(isRaster && (GetQueueState(queue)->dirtyFlags & STATE_DIRTY_RENDERTARGET) == 0u);
    }

    void VulkanGraph::RecordBufferAccess(const VulkanBindHandle* handle, VkPipelineStageFlags2 stage, VkAccessFlags2 access)
    {
        VulkanAccessRecord record;
        record.buffer.offset = handle->buffer.offset;
        record.buffer.size = handle->buffer.range;
        record.stage = stage;
        record.access = access;
        m_resourceState->RecordBufferAccess(handle->buffer.buffer, record);
    }

    bool VulkanGraph::RecordImageAccess(QueueType queue, const VulkanBindHandle* handle, bool hasLayout, VkPipelineStageFlags2 stage, VkAccessFlags2 access)
    {
        VulkanAccessRecord record;
        record.image.baseMipLevel = handle->image.range.baseMipLevel;
        record.image.levelCount = handle->image.range.levelCount;
        record.image.baseArrayLayer = handle->image.range.baseArrayLayer;
        record.image.layerCount = handle->image.range.layerCount;
        record.stage = stage;
        record.access = access;
        record.hasLayout = hasLayout;
        record.queue = GetQueueFamily(queue);
        return m_resourceState->RecordImageAccess(handle->image.image, record);
    }

    void VulkanGraph::RecordShaderCmd(QueueType queue, VulkanShaderCmd* cmd)
    {
        auto state = GetQueueState(queue);

        PK_DEBUG_FATAL_ASSERT(state->pipelineKey.shader != nullptr, "Pipeline validation failed! Shader is unassigned!");

        const auto resources = &m_globalResources;
        const auto shader = state->pipelineKey.shader;
        const auto bindPoint = VulkanEnumConvert::GetPipelineBindPoint(shader->GetStageFlags());
        const auto* descriptorLayout = shader->GetDescriptorSetLayout();
        const auto& resourceLayout = shader->GetResourceLayout();
        const auto& constantLayout = shader->GetPushConstantLayout();

        for (auto index = 0u; index < resourceLayout.GetCount(); ++index)
        {
            const auto& element = resourceLayout[index];
            const auto access = element.writeMask != 0u ? VK_ACCESS_SHADER_WRITE_BIT : VK_ACCESS_NONE;
            const auto layoutStageFlags = VulkanEnumConvert::GetPipelineStageFlags2(descriptorLayout->stageFlags);
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
                state->dirtyFlags |= PK_RENDER_STATE_DIRTY_DESCRIPTORS;
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
                    RecordImageAccess(queue, handles[i], true, layoutStageFlags, access | VK_ACCESS_2_SHADER_READ_BIT);
                }

                if (descriptor.type == ShaderResourceType::StorageBuffer)
                {
                    RecordBufferAccess(handles[i], layoutStageFlags, access | VK_ACCESS_2_SHADER_READ_BIT);
                }

                if (descriptor.type == ShaderResourceType::ConstantBuffer)
                {
                    RecordBufferAccess(handles[i], layoutStageFlags, access | VK_ACCESS_2_UNIFORM_READ_BIT);
                }
            }
        }

        if (state->descriptorCount != resourceLayout.GetCount())
        {
            state->dirtyFlags |= STATE_DIRTY_DESCRIPTORS;
            state->descriptorCount = (uint32_t)resourceLayout.GetCount();
        }

        if (state->dirtyFlags & STATE_DIRTY_DESCRIPTORS)
        {
            state->descriptorSetOffset = m_descriptorArena.AllocateDescriptorSet(descriptorLayout, state->descriptors, state->descriptorCount);
        }

        if (state->bindPoint != bindPoint)
        {
            state->dirtyFlags |= STATE_DIRTY_DESCRIPTORS;
            state->bindPoint = bindPoint;
        }

        if (state->descriptorStageFlags != descriptorLayout->stageFlags)
        {
            state->dirtyFlags |= STATE_DIRTY_DESCRIPTORS;
            state->descriptorStageFlags = descriptorLayout->stageFlags;
        }

        if (state->dirtyFlags & STATE_DIRTY_PIPELINE)
        {
            state->pipeline = m_driver->pipelineCache->GetPipeline(state->pipelineKey);
        }

        cmd->dirtyFlagMask |= STATE_DIRTY_PIPELINE | STATE_DIRTY_DESCRIPTORS;
        cmd->pipeline = state->pipeline->pipeline;
        cmd->pipelineLayout = shader->GetPipelineLayout()->layout;
        cmd->bindpoint = state->bindPoint;
        cmd->descriptorSetOffset = state->descriptorSetOffset;
        cmd->pushConstants = Allocate<uint8_t>(constantLayout.GetSize());
        cmd->pushConstantsSize = constantLayout.GetSize();
        cmd->pushConstantStageFlags = shader->GetPipelineLayout()->pushConstantStageFlags;
        
        if (cmd->pushConstantsSize)
        {
            cmd->dirtyFlagMask |= STATE_DIRTY_CONSTANTS;
            cmd->dirtyFlags |= STATE_DIRTY_CONSTANTS;
        }

        const char* data = nullptr;
        size_t dataSize = 0u;

        for (const auto& element : constantLayout)
        {
            if (resources->TryGet<char>(element.name, &data, &dataSize) && dataSize <= element.size)
            {
                memcpy(static_cast<uint8_t*>(cmd->pushConstants) + element.offset, data, dataSize);
            }
        }
    }

    void VulkanGraph::RecordRasterCmd(QueueType queue, VulkanRasterCmd* cmd)
    {
        auto state = GetQueueState(queue);

        // Validate pipeline target formats
        // Note this might be invalid if the target was accessed between draws within the same render pass.
        // Currently nothing does, Until we get that assert, lets do nothing and save some time.
        if (state->dirtyFlags & STATE_DIRTY_RENDERTARGET)
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

                    if (state->pipelineKey.fixed.sampleCountFlags != color.target->image.samples)
                    {
                        state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                        state->pipelineKey.fixed.sampleCountFlags = color.target->image.samples;
                    }

                    if (color.loadOp != LoadOp::Load)
                    {
                        RecordImageAccess(queue, color.target, false, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 0u);
                    }

                    auto hadLayout = RecordImageAccess(queue, 
                        color.target, 
                        true, 
                        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 
                        VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

                    if (color.resolve && color.resolve->image.image)
                    {
                        RecordImageAccess(queue, color.resolve, true, VK_PIPELINE_STAGE_2_RESOLVE_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
                    }

                    if (color.loadOp == LoadOp::Load && !hadLayout)
                    {
                        color.loadOp = LoadOp::Discard;
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

                depthAttachment->imageView = depth.target->image.view;
                depthAttachment->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                depthAttachment->resolveMode = depth.resolveMode;
                depthAttachment->resolveImageView = depth.resolve ? depth.resolve->image.view : nullptr;
                depthAttachment->resolveImageLayout = depth.resolve ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment->loadOp = VulkanEnumConvert::GetLoadOp(depth.loadOp);
                depthAttachment->storeOp = VulkanEnumConvert::GetStoreOp(depth.storeOp);
                depthAttachment->clearValue = VulkanEnumConvert::GetClearValue(depth.clearValue);

                if (isStencil)
                {
                    state->renderingInfo->pStencilAttachment = depthAttachment;
                }

                if (state->pipelineKey.fixed.sampleCountFlags != depth.target->image.samples)
                {
                    state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                    state->pipelineKey.fixed.sampleCountFlags = depth.target->image.samples;
                }

                if (depth.loadOp != LoadOp::Load)
                {
                    RecordImageAccess(queue, depth.target, false, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, 0u);
                }

                auto hadLayout = RecordImageAccess(queue,
                    depth.target,
                    true,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | (!isReadOnly ? VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0u));

                if (depth.loadOp == LoadOp::Load && !hadLayout)
                {
                    depth.loadOp = LoadOp::Discard;
                }

                if (isReadOnly)
                {
                    depth.storeOp = StoreOp::None;
                }
            }
        }

        if (state->dirtyFlags & STATE_DIRTY_VIEWPORTS)
        {
            state->resolvedViewports = Allocate<VkViewport>(state->viewportCount);
            Memory::Memcpy(state->resolvedViewports, state->viewports, state->viewportCount);
        }

        if (state->dirtyFlags & STATE_DIRTY_SCISSORS)
        {
            state->resolvedScissors = Allocate<VkRect2D>(state->scissorCount);
            Memory::Memcpy(state->resolvedScissors, state->scissors, state->scissorCount);
        }

        cmd->dirtyFlagMask |= STATE_DIRTY_RENDERTARGET | STATE_DIRTY_VIEWPORTS | STATE_DIRTY_SCISSORS;
        cmd->renderingInfo = state->renderingInfo;
        cmd->viewports = state->resolvedViewports;
        cmd->scissors = state->scissors;
        cmd->viewportCount = state->viewportCount;
        cmd->scissorCount = state->scissorCount;
        RecordShaderCmd(queue, cmd);
    }

    void VulkanGraph::RecordVertexCmd(QueueType queue, VulkanVertexCmd* cmd)
    {
        auto state = GetQueueState(queue);
        const auto shader = state->pipelineKey.shader;
        const auto& vertexLayout = shader->GetVertexLayout();

        auto index = 0u;
        auto validateStreams = false;

        if (state->dirtyFlags & STATE_DIRTY_SHADER)
        {
            for (; index < vertexLayout.GetCount(); ++index)
            {
                auto format = VulkanEnumConvert::GetFormat(vertexLayout[index].format);

                if (state->pipelineKey.vertexAttributes[index].location != vertexLayout[index].location ||
                    state->pipelineKey.vertexAttributes[index].format != format)
                {
                    validateStreams = true;
                    state->pipelineKey.vertexAttributes[index].location = vertexLayout[index].location;
                    state->pipelineKey.vertexAttributes[index].format = format;
                }
            }

            // Attribute count changed
            if (index < PK_RHI_MAX_VERTEX_ATTRIBUTES && state->pipelineKey.vertexAttributes[index].format != VK_FORMAT_UNDEFINED)
            {
                validateStreams = true;
                Memory::Memset(state->pipelineKey.vertexAttributes + index, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - index);
            }
        }

        if (validateStreams || (state->dirtyFlags & STATE_DIRTY_VERTEXBUFFERS))
        {
            auto streamIndex = 0u;
            auto remainingAttributes = vertexLayout.GetCount();

            for (index = 0u; index < PK_RHI_MAX_VERTEX_ATTRIBUTES && state->vertexStreamLayout[index].stride != 0; ++index)
            {
                const auto& element = state->vertexStreamLayout[index];
                auto elementIdx = 0u;

                if (state->vertexBuffers[element.stream] && vertexLayout.TryGetElement(element.name, &elementIdx))
                {
                    auto inputRate = VulkanEnumConvert::GetInputRate(element.inputRate);
                    --remainingAttributes;

                    // Merge adjacent buffer bindings if possible
                    // If the user has bound the same stream multiple times with different attributes
                    // we cannot directly use the stream index to actually index the bindings.
                    if (index > 0 &&
                        (state->vertexStreamLayout[index - 1u].stream != element.stream ||
                         state->vertexStreamLayout[index - 1u].inputRate != element.inputRate ||
                         state->vertexStreamLayout[index - 1u].stride != element.stride))
                    {
                        ++streamIndex;
                    }

                    if (state->pipelineKey.vertexStreams[streamIndex].binding != element.stream ||
                        state->pipelineKey.vertexStreams[streamIndex].inputRate != inputRate ||
                        state->pipelineKey.vertexStreams[streamIndex].stride != element.stride)
                    {
                        state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                        state->pipelineKey.vertexStreams[streamIndex].binding = element.stream;
                        state->pipelineKey.vertexStreams[streamIndex].inputRate = inputRate;
                        state->pipelineKey.vertexStreams[streamIndex].stride = element.stride;
                    }

                    if (state->pipelineKey.vertexAttributes[elementIdx].binding != streamIndex ||
                        state->pipelineKey.vertexAttributes[elementIdx].offset != element.offset)
                    {
                        state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                        state->pipelineKey.vertexAttributes[elementIdx].binding = streamIndex;
                        state->pipelineKey.vertexAttributes[elementIdx].offset = element.offset;
                    }
                }
            }

            PK_DEBUG_WARNING_ASSERT(remainingAttributes == 0, "Warning '%u' of shader vertex input attributes were not bound.", remainingAttributes);

            auto streamCount = streamIndex + 1u;

            if (streamCount < PK_RHI_MAX_VERTEX_ATTRIBUTES && state->pipelineKey.vertexStreams[streamCount].stride != 0)
            {
                state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                Memory::Memset(state->pipelineKey.vertexStreams + streamCount, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - streamCount);
            }

            state->vertexBufferCount = 0u;

            for (auto& stream : state->pipelineKey.vertexStreams)
            {
                if (stream.stride != 0)
                {
                    state->vertexBufferCount = math::max(state->vertexBufferCount, stream.binding + 1u);
                }
            }
        }

        for (auto i = 0u; i < PK_RHI_MAX_VERTEX_ATTRIBUTES && state->vertexBuffers[i]; ++i)
        {
            RecordBufferAccess(state->vertexBuffers[i], VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT);
        }

        cmd->vertexBufferCount = state->vertexBufferCount;
        cmd->vertexBuffers = Allocate<VkBuffer>(state->vertexBufferCount);
        cmd->vertexBufferOffsets = Allocate<VkDeviceSize>(state->vertexBufferCount);
        cmd->dirtyFlagMask |= state->vertexBufferCount ? STATE_DIRTY_VERTEXBUFFERS : 0u;

        for (auto& stream : state->pipelineKey.vertexStreams)
        {
            if (stream.stride != 0)
            {
                cmd->vertexBuffers[stream.binding] = state->vertexBuffers[stream.binding]->buffer.buffer;
                cmd->vertexBufferOffsets[stream.binding] = state->vertexBuffers[stream.binding]->buffer.offset;
            }
        }

        RecordRasterCmd(queue, cmd);
    }

    void VulkanGraph::RecordIndexCmd(QueueType queue, VulkanIndexCmd* cmd)
    {
        auto state = GetQueueState(queue);

        if (state->indexBuffer != nullptr)
        {
            RecordBufferAccess(state->indexBuffer, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_ACCESS_2_INDEX_READ_BIT);
            cmd->dirtyFlagMask |= STATE_DIRTY_INDEXBUFFER;
            cmd->indexBuffer = state->indexBuffer->buffer.buffer;
            cmd->indexBufferOffset = state->indexBuffer->buffer.offset;
            cmd->indexType = state->indexType;
        }

        RecordVertexCmd(queue, cmd);
    }

    void VulkanGraph::RecordFlags(QueueType queue, VulkanCmd* cmd, VkPipelineStageFlags2 stageFlags)
    {
        auto state = GetQueueState(queue);
        cmd->dirtyFlags = state->dirtyFlags & cmd->dirtyFlagMask;
        cmd->stageFlags = stageFlags;
        state->dirtyFlags &= ~cmd->dirtyFlagMask;
    }
}
