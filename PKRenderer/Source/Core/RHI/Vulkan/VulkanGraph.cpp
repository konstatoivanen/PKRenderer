#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
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
        m_descriptorHeap(driver->descriptorHeap),
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

    #define PK_VK_BIND_VIEWS(name, assigner, count)\
    auto* handles = PK_STACK_ALLOC(VulkanResourceView, count);\
    for (auto i = 0u; i < (uint32_t)count; ++i) handles[i] = assigner;\
    m_globalResources.Set(name, handles, (uint32_t)count)                

    void VulkanGraph::SetBuffers(NameID name, RHIBuffer** buffers, const BufferIndexRange* ranges, size_t count) { PK_VK_BIND_VIEWS(name, static_cast<const VulkanBuffer*>(buffers[i])->GetResourceView(ranges[i]), count); }
    void VulkanGraph::SetTextures(NameID name, RHITexture** textures, const TextureViewRange* ranges, size_t count) { PK_VK_BIND_VIEWS(name, static_cast<const VulkanTexture*>(textures[i])->GetResourceView(ranges[i], TextureViewMode::SRV), count); }
    void VulkanGraph::SetImages(NameID name, RHITexture** images, const TextureViewRange* ranges, size_t count) { PK_VK_BIND_VIEWS(name, static_cast<const VulkanTexture*>(images[i])->GetResourceView(ranges[i], TextureViewMode::UAV), count); }
    void VulkanGraph::SetSamplers(NameID name, const SamplerDescriptor* samplers, size_t count) { PK_VK_BIND_VIEWS(name, m_descriptorHeap->GetSamplerView(samplers[i]), count); }
    void VulkanGraph::SetAccelerationStructures(NameID name, RHIAccelerationStructure** structures, size_t count) { PK_VK_BIND_VIEWS(name, static_cast<VulkanAccelerationStructure*>(structures[i])->GetResourceView(), count); }
    void VulkanGraph::SetConstant(NameID name, const void* data, uint32_t size) { m_globalResources.Set<char>(name, static_cast<const char*>(data), size); }
    void VulkanGraph::SetKeyword(NameID name, bool value) { m_globalResources.Set<bool>(name, value); }

    #undef PK_VK_BIND_VIEWS

    uint32_t VulkanGraph::MakeResident(RHITexture* texture)
    {
        const auto& view = *texture->GetNativeView<VulkanResourceView>();
        AddResidencyBarrierPass();
        RecordImageAccess(QueueType::EnumCount, view, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_SHADER_READ_BIT);
        return m_descriptorHeap->GetBindlessIndex(view, ShaderResourceType::Texture);
    }

    uint32_t VulkanGraph::MakeResident(RHIBuffer* buffer)
    {
        const auto& view = *buffer->GetNativeView<VulkanResourceView>();
        AddResidencyBarrierPass();
        RecordMemoryAccess(QueueType::EnumCount, view, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT);
        return m_descriptorHeap->GetBindlessIndex(view, ShaderResourceType::StorageBuffer);
    }

    uint32_t VulkanGraph::MakeResident(const SamplerDescriptor& sampler) { return m_descriptorHeap->GetSamplerIndex(sampler); }
    uint3 VulkanGraph::GetResidentTextureSize(uint32_t bindlessIndex) { return m_descriptorHeap->GetTextureSize(bindlessIndex); }
    
    RHIBuffer* VulkanGraph::AcquireStagingBuffer(size_t size) { return m_stagingArena->BeginWrite(size); }
    void VulkanGraph::ReleaseStagingBuffer(RHIBuffer* buffer) { m_stagingArena->EndWrite(buffer); }


    void VulkanGraph::SetViewPorts(const uint4* rects, uint32_t count)
    {
        count = math::min(count, PK_RHI_MAX_VIEWPORTS);
        auto& dynamic = m_pipelineState.dynamicState;
        VkViewport viewports[PK_RHI_MAX_VIEWPORTS];

        for (auto i = 0u; i < count; ++i)
        {
            auto& rect = rects[i];
            viewports[i] = { (float)rect.x, (float)rect.y, (float)rect.z, (float)rect.w, 0.0f, 1.0f };
        }

        if (dynamic.viewportCount != count || (count && memcmp(dynamic.viewports, viewports, sizeof(VkViewport) * count) != 0))
        {
            auto* resolved = Allocate<VkViewport>(count);
            memcpy(resolved, viewports, sizeof(VkViewport) * count);
            dynamic.viewports = resolved;
            dynamic.viewportCount = (uint16_t)count;
            m_pipelineState.dirtyFlags |= STATE_DIRTY_DYNAMIC_STATE;
        }
    }

    void VulkanGraph::SetScissors(const uint4* rects, uint32_t count)
    {
        count = math::min(count, PK_RHI_MAX_VIEWPORTS);
        auto& dynamic = m_pipelineState.dynamicState;

        if (dynamic.scissorCount != count || (count > 0u && memcmp(dynamic.scissors, rects, sizeof(VkRect2D) * count) != 0))
        {
            auto* resolved = Allocate<VkRect2D>(count);
            memcpy(resolved, rects, sizeof(VkRect2D) * count);
            dynamic.scissors = resolved;
            dynamic.scissorCount = (uint16_t)count;
            m_pipelineState.dirtyFlags |= STATE_DIRTY_DYNAMIC_STATE;
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
            auto* binding = &bindings[i];
            const auto& desc = binding->target->GetDescriptor();
            const auto format = VulkanEnumConvert::GetFormat(desc.format);
            const auto samples = VulkanEnumConvert::GetSampleCountFlags(desc.samples);
            const auto isDepth = VulkanEnumConvert::IsDepthFormat(format);

            auto* attachment = isDepth ? &renderTarget.depth : (renderTarget.colors + renderTarget.colorCount++);
            attachment->target = static_cast<const VulkanTexture*>(binding->target)->GetResourceView(binding->targetRange, TextureViewMode::RTV);
            attachment->resolve = binding->resolve ? static_cast<const VulkanTexture*>(binding->resolve)->GetResourceView(binding->resolveRange, TextureViewMode::RTV) : VulkanResourceView{};
            attachment->format = format;
            attachment->samples = samples;
            attachment->loadOp = binding->loadOp;
            attachment->storeOp = binding->storeOp;
            attachment->clearValue = binding->clearValue;
            attachment->resolveMode = binding->resolve ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
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
        auto& fixed = m_pipelineState.pipelineKey.fixed;
        auto& dynamic = m_pipelineState.dynamicState;
        
        if (fixed.depthClampEnable != rasterization.depthClampEnable ||
            fixed.polygonMode != rasterization.polygonMode ||
            fixed.rasterMode != rasterization.rasterMode ||
            fixed.topology != rasterization.topology ||
            fixed.overEstimation != rasterization.overEstimation)
        {
            fixed.depthClampEnable = rasterization.depthClampEnable;
            fixed.polygonMode = rasterization.polygonMode;
            fixed.rasterMode = rasterization.rasterMode;
            fixed.topology = rasterization.topology;
            fixed.overEstimation = rasterization.overEstimation;
            m_pipelineState.dirtyFlags |= STATE_DIRTY_PIPELINE;
        }

        if (dynamic.cullMode != rasterization.cullMode ||
            dynamic.frontFace != rasterization.frontFace ||
            dynamic.rasterizerDiscardEnable != rasterization.rasterizerDiscardEnable ||
            dynamic.depthBiasEnable != rasterization.depthBiasEnable ||
            dynamic.depthBiasConstantFactor != rasterization.depthBiasConstantFactor ||
            dynamic.depthBiasClamp != rasterization.depthBiasClamp ||
            dynamic.depthBiasSlopeFactor != rasterization.depthBiasSlopeFactor ||
            dynamic.lineWidth != rasterization.lineWidth)
        {
            dynamic.cullMode = rasterization.cullMode;
            dynamic.frontFace = rasterization.frontFace;
            dynamic.rasterizerDiscardEnable = rasterization.rasterizerDiscardEnable;
            dynamic.depthBiasEnable = rasterization.depthBiasEnable;
            dynamic.depthBiasConstantFactor = rasterization.depthBiasConstantFactor;
            dynamic.depthBiasClamp = rasterization.depthBiasClamp;
            dynamic.depthBiasSlopeFactor = rasterization.depthBiasSlopeFactor;
            dynamic.lineWidth = rasterization.lineWidth;
            m_pipelineState.dirtyFlags |= STATE_DIRTY_DYNAMIC_STATE;
        }
    }

    void VulkanGraph::SetDepthStencil(const DepthStencilParameters& depthStencil)
    {
        if (memcmp(&m_pipelineState.dynamicState.depthStencil, &depthStencil, sizeof(DepthStencilParameters)) != 0)
        {
            m_pipelineState.dynamicState.depthStencil = depthStencil;
            m_pipelineState.dirtyFlags |= STATE_DIRTY_DYNAMIC_STATE;
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
            PK_DEBUG_FATAL_ASSERT(buffers[i] != nullptr, "Passing null vertex buffer is not allowed!");
            const auto& view = *buffers[i]->GetNativeView<VulkanResourceView>();

            if (view != m_pipelineState.vertexBuffers[i])
            {
                m_pipelineState.dirtyFlags |= STATE_DIRTY_VERTEXBUFFERS;
                m_pipelineState.vertexBuffers[i] = view;
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
        const auto view = buffer ? *buffer->GetNativeView<VulkanResourceView>() : VulkanResourceView{};
        const auto indexType = VulkanEnumConvert::GetIndexType(indexSize);

        if (m_pipelineState.indexBuffer != view || (buffer != nullptr && m_pipelineState.indexType != indexType))
        {
            m_pipelineState.indexBuffer = view;
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
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->dynamicState);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->descriptorHeapOffsets, cmd->pushConstants, cmd->pushConstantSize);
            ctx->cmb->SetVertexBuffers(cmd->vertexBuffers, cmd->vertexBufferCount);
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
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->dynamicState);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->descriptorHeapOffsets, cmd->pushConstants, cmd->pushConstantSize);
            ctx->cmb->SetVertexBuffers(cmd->vertexBuffers, cmd->vertexBufferCount);
            vkCmdDrawIndirect2KHR(*ctx->cmb, &cmd->indirectInfo);
        });

        auto indirectHandle = indirectArguments->GetNativeView<VulkanResourceView>();
        cmd->passIndex = AddBarrierPass(QueueType::Graphics, true);
        cmd->indirectInfo.sType = VK_STRUCTURE_TYPE_DRAW_INDIRECT_2_INFO_KHR;
        cmd->indirectInfo.addressRange = { indirectHandle->memory.address + offset, (VkDeviceSize)drawCount * stride, stride };
        cmd->indirectInfo.addressFlags = PK_VK_ADDRESS_FLAGS;
        cmd->indirectInfo.drawCount = drawCount;
        RecordVertexCmd(cmd);
        RecordMemoryAccess(QueueType::Graphics, *indirectHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanGraphContext* ctx, VulkanDrawIndexedCmd* cmd)
        {
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->dynamicState);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->descriptorHeapOffsets, cmd->pushConstants, cmd->pushConstantSize);
            ctx->cmb->SetVertexBuffers(cmd->vertexBuffers, cmd->vertexBufferCount);
            ctx->cmb->SetIndexBuffer(cmd->indexInfo);
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
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->dynamicState);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->descriptorHeapOffsets, cmd->pushConstants, cmd->pushConstantSize);
            ctx->cmb->SetVertexBuffers(cmd->vertexBuffers, cmd->vertexBufferCount);
            ctx->cmb->SetIndexBuffer(cmd->indexInfo);
            vkCmdDrawIndexedIndirect2KHR(*ctx->cmb, &cmd->indirectInfo);
        });

        auto indirectHandle = indirectArguments->GetNativeView<VulkanResourceView>();
        cmd->passIndex = AddBarrierPass(QueueType::Graphics, true);
        cmd->indirectInfo.sType = VK_STRUCTURE_TYPE_DRAW_INDIRECT_2_INFO_KHR;
        cmd->indirectInfo.addressRange = { indirectHandle->memory.address + offset, (VkDeviceSize)drawCount * stride, stride };
        cmd->indirectInfo.addressFlags = PK_VK_ADDRESS_FLAGS;
        cmd->indirectInfo.drawCount = drawCount;
        RecordIndexCmd(cmd);
        RecordMemoryAccess(QueueType::Graphics, *indirectHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::DrawMeshTasks(const uint3& dimensions)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanGraphContext* ctx, VulkanDrawMeshTasksCmd* cmd)
        {
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->dynamicState);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->descriptorHeapOffsets, cmd->pushConstants, cmd->pushConstantSize);
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
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->dynamicState);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->descriptorHeapOffsets, cmd->pushConstants, cmd->pushConstantSize);
            vkCmdDrawMeshTasksIndirect2EXT(*ctx->cmb, &cmd->indirectInfo);
        });

        auto indirectHandle = indirectArguments->GetNativeView<VulkanResourceView>();
        cmd->passIndex = AddBarrierPass(QueueType::Graphics, true);
        cmd->indirectInfo.sType = VK_STRUCTURE_TYPE_DRAW_INDIRECT_2_INFO_KHR;
        cmd->indirectInfo.addressRange = { indirectHandle->memory.address + offset, (VkDeviceSize)drawCount * stride, stride };
        cmd->indirectInfo.addressFlags = PK_VK_ADDRESS_FLAGS;
        cmd->indirectInfo.drawCount = drawCount;
        RecordRasterCmd(cmd);
        RecordMemoryAccess(QueueType::Graphics, *indirectHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::DrawMeshTasksIndirectCount(const RHIBuffer* indirectArguments, size_t offset, const RHIBuffer* countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride)
    {
        auto cmd = AllocateCommand(QueueType::Graphics, [](VulkanGraphContext* ctx, VulkanDrawMeshTasksIndirectCountCmd* cmd)
        {
            ctx->cmb->SetRasterState(cmd->renderingInfo, cmd->dynamicState);
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->descriptorHeapOffsets, cmd->pushConstants, cmd->pushConstantSize);
            vkCmdDrawMeshTasksIndirectCount2EXT(*ctx->cmb, &cmd->indirectCountInfo);
        });

        auto indirectHandle = indirectArguments->GetNativeView<VulkanResourceView>();
        auto countHandle = countBuffer->GetNativeView<VulkanResourceView>();
        cmd->passIndex = AddBarrierPass(QueueType::Graphics, true);
        cmd->indirectCountInfo.sType = VK_STRUCTURE_TYPE_DRAW_INDIRECT_COUNT_2_INFO_KHR;
        cmd->indirectCountInfo.addressRange = { indirectHandle->memory.address + offset, (VkDeviceSize)maxDrawCount * stride, stride };
        cmd->indirectCountInfo.addressFlags = PK_VK_ADDRESS_FLAGS;
        cmd->indirectCountInfo.countAddressRange = { countHandle->memory.address + countOffset, sizeof(uint32_t) };
        cmd->indirectCountInfo.countAddressFlags = PK_VK_ADDRESS_FLAGS;
        cmd->indirectCountInfo.maxDrawCount = maxDrawCount;
        RecordRasterCmd(cmd);
        RecordMemoryAccess(QueueType::Graphics, *indirectHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
        RecordMemoryAccess(QueueType::Graphics, *countHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::Dispatch(QueueType queue, const uint3& dimensions)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanDispatchCmd* cmd)
        {
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->descriptorHeapOffsets, cmd->pushConstants, cmd->pushConstantSize);
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
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->descriptorHeapOffsets, cmd->pushConstants, cmd->pushConstantSize);
            vkCmdDispatchIndirect2KHR(*ctx->cmb, &cmd->indirectInfo);
        });

        auto indirectHandle = indirectArguments->GetNativeView<VulkanResourceView>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->indirectInfo.sType = VK_STRUCTURE_TYPE_DISPATCH_INDIRECT_2_INFO_KHR;
        cmd->indirectInfo.addressRange = { indirectHandle->memory.address + offset, sizeof(VkDispatchIndirectCommand) };
        cmd->indirectInfo.addressFlags = PK_VK_ADDRESS_FLAGS;
        RecordShaderCmd(queue, cmd);
        RecordMemoryAccess(queue, *indirectHandle, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
    }

    void VulkanGraph::DispatchRays(QueueType queue, const uint3& dimensions)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanDispatchRaysCmd* cmd)
        {
            ctx->cmb->SetPipelineState(cmd->pipeline, cmd->descriptorHeapOffsets, cmd->pushConstants, cmd->pushConstantSize);
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
            auto handle = cmd->dstSwapchain->GetResourceView();
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

        const auto* view = src->GetNativeView<VulkanResourceView>();
        auto srcRes = src->GetResolution();
        auto dstRes = dst->GetResolution();
        auto minres = math::min(srcRes, dstRes);
        auto diff = int3(srcRes - minres);
        auto srcMin = diff / 2;
        auto srcMax = int3(srcRes) - (diff - srcMin);

        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->src = view->image.image;
        cmd->dstSwapchain = static_cast<VulkanSwapchain*>(dst);
        cmd->region.srcSubresource = { (uint32_t)view->image.range.aspectMask, 0u, 0u, 1u };
        cmd->region.srcOffsets[0] = { srcMin.x, srcMax.y, srcMin.z };
        cmd->region.srcOffsets[1] = { srcMax.x, srcMin.y, srcMax.z };
        cmd->region.dstOffsets[0] = { 0, 0, 0 };
        cmd->region.dstOffsets[1] = { (int)dstRes.x, (int)dstRes.y, (int)dstRes.z };
        cmd->filter = VulkanEnumConvert::GetFilterMode(filter);

        RecordImageAccess(queue, *view, VK_PIPELINE_STAGE_2_BLIT_BIT_KHR, VK_ACCESS_2_TRANSFER_READ_BIT);
    }

    void VulkanGraph::Blit(QueueType queue, RHISwapchain* src, RHIBuffer* dst)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanBlitSwapchainToBufferCmd* cmd)
        {
            auto handle = cmd->src->GetResourceView();
            auto signal = cmd->src->ConsumeAcquireSignal();

            if (signal != VK_NULL_HANDLE)
            {
                ctx->queue->AddWait(signal, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 0ull);
            }

            VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
            barrier.srcStageMask = signal ? VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT : (VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT);
            barrier.srcAccessMask = signal ? 0u : VK_ACCESS_2_TRANSFER_WRITE_BIT;
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

            cmd->region.sType = VK_STRUCTURE_TYPE_DEVICE_MEMORY_IMAGE_COPY_KHR;
            cmd->region.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            cmd->region.imageOffset = { 0, 0, 0 };
            cmd->region.imageExtent = handle->image.extent;
            cmd->region.imageSubresource.aspectMask = handle->image.range.aspectMask;
            cmd->region.imageSubresource.mipLevel = 0u;
            cmd->region.imageSubresource.baseArrayLayer = 0u;
            cmd->region.imageSubresource.layerCount = VK_REMAINING_ARRAY_LAYERS;

            ctx->cmb->Barrier(&barrier);

            VkCopyDeviceMemoryImageInfoKHR copy{ VK_STRUCTURE_TYPE_COPY_DEVICE_MEMORY_IMAGE_INFO_KHR };
            copy.image = handle->image.image;
            copy.regionCount = 1u;
            copy.pRegions = &cmd->region;
            vkCmdCopyImageToMemoryKHR(*ctx->cmb, &copy);
        });

        auto handle = dst->GetNativeView<VulkanResourceView>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->src = static_cast<VulkanSwapchain*>(src);
        cmd->region.addressRange = { handle->memory.address, handle->memory.size };
        cmd->region.addressFlags = PK_VK_ADDRESS_FLAGS;
        RecordMemoryAccess(queue, *handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
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

        auto srcView = static_cast<VulkanTexture*>(src)->GetResourceView(srcRange, TextureViewMode::RAW);
        auto dstView = static_cast<VulkanTexture*>(dst)->GetResourceView(dstRange, TextureViewMode::RAW);
        auto srcLayers = math::min(srcView.image.range.layerCount, src->GetLayers());
        auto dstLayers = math::min(dstView.image.range.layerCount, dst->GetLayers());

        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->src = srcView.image.image;
        cmd->dst = dstView.image.image;
        cmd->srcFormat = srcView.image.format;
        cmd->dstFormat = dstView.image.format;
        cmd->srcSamples = srcView.image.samples;
        cmd->dstSamples = dstView.image.samples;
        cmd->region.srcSubresource = { (uint32_t)srcView.image.range.aspectMask, srcView.image.range.baseMipLevel, srcView.image.range.baseArrayLayer, 0u };
        cmd->region.dstSubresource = { (uint32_t)srcView.image.range.aspectMask, dstView.image.range.baseMipLevel, dstView.image.range.baseArrayLayer, 0u };
        cmd->region.srcOffsets[1] = { (int)srcView.image.extent.width, (int)srcView.image.extent.height, (int)srcView.image.extent.depth };
        cmd->region.dstOffsets[1] = { (int)dstView.image.extent.width, (int)dstView.image.extent.height, (int)dstView.image.extent.depth };
        cmd->region.dstSubresource.layerCount = cmd->region.srcSubresource.layerCount = math::min(srcLayers, dstLayers);
        cmd->filter = VulkanEnumConvert::GetFilterMode(filter);
        
        RecordImageAccess(queue, srcView, VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
        RecordImageAccess(queue, dstView, VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
        EndDebugScope();
    }

    void VulkanGraph::Clear(QueueType queue, RHIBuffer* dst, size_t offset, size_t size, uint32_t value)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanFillBufferCmd* cmd)
        {
            vkCmdFillMemoryKHR(*ctx->cmb, &cmd->range, PK_VK_ADDRESS_FLAGS, cmd->value);
        });

        auto handle = dst->GetNativeView<VulkanResourceView>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->range.address = handle->memory.address + offset;
        cmd->range.size = size;
        cmd->value = value;
        RecordMemoryAccess(queue, *handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
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

        auto view = static_cast<VulkanTexture*>(dst)->GetResourceView(range, TextureViewMode::RAW);
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->image = view.image.image;
        cmd->range = view.image.range;
        cmd->clearValue = VulkanEnumConvert::GetClearValue(value);
        RecordImageAccess(queue, view, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT);
    }

    void VulkanGraph::UpdateBuffer(QueueType queue, RHIBuffer* dst, size_t offset, size_t size, const void* data)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanUpdateBufferCmd* cmd)
        {
            vkCmdUpdateMemoryKHR(*ctx->cmb, &cmd->range, PK_VK_ADDRESS_FLAGS, cmd->range.size, cmd->pData);
        });

        auto handle = dst->GetNativeView<VulkanResourceView>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->range.address = handle->memory.address + offset;
        cmd->range.size = size;
        cmd->pData = Allocate<uint8_t>(size);
        memcpy(cmd->pData, data, size);
        RecordMemoryAccess(queue, *handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT);
    }

    void VulkanGraph::CopyBuffer(QueueType queue, RHIBuffer* dst, RHIBuffer* src, size_t srcOffset, size_t dstOffset, size_t size)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanCopyBufferCmd* cmd)
        {
            VkCopyDeviceMemoryInfoKHR copy{ VK_STRUCTURE_TYPE_COPY_DEVICE_MEMORY_INFO_KHR };
            copy.regionCount = 1u;
            copy.pRegions = &cmd->region;
            vkCmdCopyMemoryKHR(*ctx->cmb, &copy);
        });

        auto srcHandle = src->GetNativeView<VulkanResourceView>();
        auto dstHandle = dst->GetNativeView<VulkanResourceView>();
        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->region.sType = VK_STRUCTURE_TYPE_DEVICE_MEMORY_COPY_KHR;
        cmd->region.srcRange = { srcHandle->memory.address + srcOffset, size };
        cmd->region.srcFlags = PK_VK_ADDRESS_FLAGS;
        cmd->region.dstRange = { dstHandle->memory.address + dstOffset, size };
        cmd->region.dstFlags = PK_VK_ADDRESS_FLAGS;

        // Assume always staging source?!?
        RecordMemoryAccess(queue, *dstHandle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    }

    void VulkanGraph::CopyToTexture(QueueType queue, RHITexture* texture, RHIBuffer* buffer, TextureDataRegion* regions, uint32_t regionCount)
    {
        auto cmd = AllocateCommand(queue, [](VulkanGraphContext* ctx, VulkanCopyBufferToImageCmd* cmd)
        {
            VkCopyDeviceMemoryImageInfoKHR copy{ VK_STRUCTURE_TYPE_COPY_DEVICE_MEMORY_IMAGE_INFO_KHR };
            copy.image = cmd->dst;
            copy.regionCount = cmd->regionCount;
            copy.pRegions = cmd->regions;
            vkCmdCopyMemoryToImageKHR(*ctx->cmb, &copy);
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

        auto srcView = buffer->GetNativeView<VulkanResourceView>();
        auto dstView = static_cast<VulkanTexture*>(texture)->GetResourceView(resourceRange, TextureViewMode::RAW);

        cmd->passIndex = AddBarrierPass(queue, false);
        cmd->dst = dstView.image.image;
        cmd->regions = Allocate<VkDeviceMemoryImageCopyKHR>(regionCount);
        cmd->regionCount = regionCount;

        const auto dstFormat = VulkanEnumConvert::GetFormat(texture->GetDescriptor().format);
        const auto blockSize = VulkanEnumConvert::GetFormatBlockSize(dstFormat);
        const auto isBlockCompressed = VulkanEnumConvert::IsBlockCompressedFormat(dstFormat);
        const auto blockExtent = isBlockCompressed ? 4ull : 1ull;

        for (auto i = 0u; i < regionCount; ++i)
        {
            auto& region = regions[i];
            const VkDeviceSize blocksX = (region.extent.x + blockExtent - 1ull) / blockExtent;
            const VkDeviceSize blocksY = (region.extent.y + blockExtent - 1ull) / blockExtent;
            const VkDeviceSize totalBytes = blocksX * blocksY * blockSize * region.extent.z * region.layers;

            cmd->regions[i].sType = VK_STRUCTURE_TYPE_DEVICE_MEMORY_IMAGE_COPY_KHR;
            cmd->regions[i].pNext = nullptr;
            cmd->regions[i].addressRange.address = srcView->memory.address + region.bufferOffset;
            cmd->regions[i].addressRange.size = totalBytes;
            cmd->regions[i].addressFlags = PK_VK_ADDRESS_FLAGS;
            cmd->regions[i].addressRowLength = 0u;
            cmd->regions[i].addressImageHeight = 0u;
            cmd->regions[i].imageSubresource.aspectMask = dstView.image.range.aspectMask;
            cmd->regions[i].imageSubresource.mipLevel = region.level;
            cmd->regions[i].imageSubresource.baseArrayLayer = region.layer;
            cmd->regions[i].imageSubresource.layerCount = region.layers;
            cmd->regions[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            cmd->regions[i].imageOffset.x = region.offset.x;
            cmd->regions[i].imageOffset.y = region.offset.y;
            cmd->regions[i].imageOffset.z = region.offset.z;
            cmd->regions[i].imageExtent.width = region.extent.x;
            cmd->regions[i].imageExtent.height = region.extent.y;
            cmd->regions[i].imageExtent.depth = region.extent.z;
        }
        
        // Assume always staging source?!?
        RecordImageAccess(queue, dstView, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    }

    RHIAccelerationStructureBuilder* VulkanGraph::BeginAccelerationStructureWrite(RHIAccelerationStructure* structure, size_t instanceLimit)
    {
        auto vkStructure = static_cast<VulkanAccelerationStructure*>(structure);
        auto writer = AllocateNew<VulkanAccelerationStructureBuilder>();
        auto inputBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * instanceLimit;
        writer->structure = vkStructure;
        writer->stagingBuffer = AcquireStagingBuffer(inputBufferSize);
        writer->instances = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(writer->stagingBuffer->GetMappedPtr());
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
            vkStructure->buffer = RHI::CreateBuffer(bufferSize, BufferUsage::Vram | BufferUsage::AccelerationStructure, name.c_str());

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
            vkStructure->view.memory.address = vkStructure->structure.deviceAddress;
            vkStructure->view.memory.size = vkStructure->structure.size;
            vkStructure->view.UpdateHashMemory();
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

        if (rebuildTLAS)
        {
            cmd->passIndex = AddBarrierPass(queue, false);

            RecordMemoryAccess(queue, 
                vkStructure->view, 
                VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR,
                VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR);
        }
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
            auto handle = cmd->swapchain->GetResourceView();
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
        queues->SetDescriptorHeaps(m_descriptorHeap->GetResourceHeapInfo(), m_descriptorHeap->GetSamplerHeapInfo());

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
        auto completedPassCounter = scope.lastPassCounter;

        PK_FATAL_ASSERT(scope.fence.Wait(UINT64_MAX), "Failed to acquire new graph scope!");

        for (auto i = 0u; i < PK_VK_MAX_GRAPH_SCOPES; ++i)
        {
            if (m_scopes[i].fence.IsValid() && m_scopes[i].fence.IsComplete())
            {
                m_timerArena->FlushTimeline(m_scopes[i].timerTimelineIndex);
                m_stagingArena->FreeRange(m_scopes[i].stagingRangeIndex);
                m_descriptorHeap->FreeRange(m_scopes[i].descriptorRangeIndex);
                m_scopes[i].fence.Invalidate();
            }
        }

        m_pipelineState = PipelineState();
        m_stagingArena->BeginRange();
        m_descriptorHeap->BeginRange();
        m_accessRecorder.ResetPasses(completedPassCounter);
        RHIGraph::Clear();
    }

    VulkanGraphScope& VulkanGraph::EndGraph()
    {
        auto& scope = m_scopes[m_invocationCounter % PK_VK_MAX_GRAPH_SCOPES];
        scope.stagingRangeIndex = m_stagingArena->EndRange();
        scope.descriptorRangeIndex = m_descriptorHeap->EndRange();
        scope.lastPassCounter = m_accessRecorder.GetPassCounter();
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


    void VulkanGraph::AddResidencyBarrierPass()
    {
        if ((m_pipelineState.coalescingQueue == ~0u || (m_pipelineState.dirtyFlags & STATE_DIRTY_RENDERTARGET) != 0u) && m_pipelineState.residencyPass == 0ull)
        {
            m_pipelineState.residencyPass = m_accessRecorder.AddPass();
        }
    }

    uint64_t VulkanGraph::AddBarrierPass(QueueType queue, bool isRaster)
    {
        auto queueIndex = m_driver->queues->GetQueueIndex(queue);

        if (!isRaster)
        {
            m_pipelineState.coalescingQueue = ~0u;
            const auto pass = m_pipelineState.residencyPass ? m_pipelineState.residencyPass : m_accessRecorder.AddPass();
            m_pipelineState.residencyPass = 0ull;
            return pass;
        }

        if (m_pipelineState.coalescingQueue != queueIndex || (m_pipelineState.dirtyFlags & STATE_DIRTY_RENDERTARGET) != 0u)
        {
            m_pipelineState.coalescingQueue = queueIndex;
            const auto pass = m_pipelineState.residencyPass ? m_pipelineState.residencyPass : m_accessRecorder.AddPass();
            m_pipelineState.residencyPass = 0ull;
            return pass;
        }

        return 0u;
    }

    void VulkanGraph::RecordMemoryAccess(QueueType queue, const VulkanResourceView& view, VkPipelineStageFlags2 stage, VkAccessFlags2 access)
    {
        VulkanAccessRecord record{};
        record.memory = view.memory;
        record.stage = stage;
        record.access = access;
        record.isLocked = queue == QueueType::EnumCount;
        record.queueMask = record.isLocked ? 0xFFFFu : (uint16_t)(1u << m_driver->queues->GetQueueIndex(queue));
        m_accessRecorder.RecordMemoryAccess(record);
    }

    void VulkanGraph::RecordImageAccess(QueueType queue, const VulkanResourceView& view, VkPipelineStageFlags2 stage, VkAccessFlags2 access)
    {
        VulkanAccessRecord record{};
        record.image.baseMipLevel = view.image.range.baseMipLevel;
        record.image.levelCount = view.image.range.levelCount;
        record.image.baseArrayLayer = view.image.range.baseArrayLayer;
        record.image.layerCount = view.image.range.layerCount;
        record.stage = stage;
        record.access = access;
        record.isLocked = queue == QueueType::EnumCount;
        record.queueMask = record.isLocked ? 0xFFFFu : (uint16_t)(1u << m_driver->queues->GetQueueIndex(queue));
        m_accessRecorder.RecordImageAccess(view.image.image, record);
    }


    void VulkanGraph::RecordShaderCmd(QueueType queue, VulkanShaderCmd* cmd)
    {
        auto state = &m_pipelineState;
        const auto resources = &m_globalResources;
        const auto shader = state->pipelineKey.shader;

        PK_DEBUG_FATAL_ASSERT(shader != nullptr, "Pipeline validation failed! Shader is unassigned!");

        const auto& resourceLayout = shader->GetResourceLayout();
        const auto layoutStageFlags = VulkanEnumConvert::GetPipelineStageFlags(shader->GetStageFlags());

        for (auto index = 0u; index < resourceLayout.GetCount(); ++index)
        {
            const auto& element = resourceLayout[index];
            const auto access = element.writeMask != 0u ? VK_ACCESS_2_SHADER_WRITE_BIT : VK_ACCESS_2_NONE;

            auto& descriptor = state->descriptors[index];
            const VulkanResourceView* views = nullptr;
            auto size = 0ull;

            PK_FATAL_ASSERT(resources->TryGet<VulkanResourceView>(element.name, &views, &size), "Descriptor '%s' not bound!", element.name.c_str());
            
            const auto count = (uint32_t)(size / sizeof(VulkanResourceView));
            PK_FATAL_ASSERT(count == element.count, "Descriptor '%s' bound array size '%u' doesn't match size '%u' in shader", element.name.c_str(), count, element.count);

            auto hash = views[0].hash;

            for (auto i = 1u; i < count; ++i)
            {
                hash = Hash::Mix(hash, views[i].hash);
            }

            if (descriptor.views != views || descriptor.count != count || descriptor.type != element.type || descriptor.hash != hash)
            {
                state->dirtyFlags |= STATE_DIRTY_DESCRIPTORS;
                descriptor.views = views;
                descriptor.count = (uint16_t)count;
                descriptor.type = element.type;
                descriptor.hash = hash;
            }

            for (auto i = 0u; i < count; ++i)
            {
                if (descriptor.type == ShaderResourceType::SamplerTexture || 
                    descriptor.type == ShaderResourceType::Texture || 
                    descriptor.type == ShaderResourceType::Image)
                {
                    RecordImageAccess(queue, views[i], layoutStageFlags, access | VK_ACCESS_2_SHADER_READ_BIT);
                }

                if (descriptor.type == ShaderResourceType::StorageBuffer)
                {
                    RecordMemoryAccess(queue, views[i], layoutStageFlags, access | VK_ACCESS_2_SHADER_READ_BIT);
                }

                if (descriptor.type == ShaderResourceType::ConstantBuffer)
                {
                    RecordMemoryAccess(queue, views[i], layoutStageFlags, access | VK_ACCESS_2_UNIFORM_READ_BIT);
                }

                if (descriptor.type == ShaderResourceType::AccelerationStructure)
                {
                    RecordMemoryAccess(queue, views[i], layoutStageFlags, VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR);
                }
            }
        }

        if (state->descriptorCount != resourceLayout.GetCount() || (state->dirtyFlags & STATE_DIRTY_SHADER) != 0u)
        {
            state->dirtyFlags |= STATE_DIRTY_DESCRIPTORS;
            state->descriptorCount = (uint32_t)resourceLayout.GetCount();
        }

        if (state->dirtyFlags & STATE_DIRTY_DESCRIPTORS)
        {
            state->descriptorHeapOffsets = m_descriptorHeap->AllocateDescriptorSet(shader.value, state->descriptors, state->descriptorCount);
        }

        if (state->dirtyFlags & STATE_DIRTY_PIPELINE)
        {
            state->pipeline = m_driver->pipelineCache->GetPipeline(state->pipelineKey);
        }

        state->dirtyFlags &= ~(STATE_DIRTY_PIPELINE | STATE_DIRTY_DESCRIPTORS | STATE_DIRTY_SHADER);

        cmd->pipeline = state->pipeline;
        cmd->descriptorHeapOffsets = state->descriptorHeapOffsets;
        cmd->pushConstantSize = shader->GetPushConstantSize();
        cmd->pushConstants = Allocate<uint8_t>(cmd->pushConstantSize);

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
        const bool isDirtyDynamic = !state->resolvedDynamicState || (state->dirtyFlags & STATE_DIRTY_DYNAMIC_STATE) != 0u;
        state->dirtyFlags &= ~(STATE_DIRTY_RENDERTARGET | STATE_DIRTY_DYNAMIC_STATE);

        if (isDirtyRenderTarget || isNewPass)
        {
            auto& colors = state->renderTarget.colors;
            auto& depth = state->renderTarget.depth;
            const auto colorCount = state->renderTarget.colorCount;
            const auto depthFormat = depth.target.image.image ? depth.format : VK_FORMAT_UNDEFINED;

            auto colorAttachments = Allocate<VkRenderingAttachmentInfo>(colorCount);
            auto depthAttachment = Allocate<VkRenderingAttachmentInfo>(depthFormat != VK_FORMAT_UNDEFINED);

            state->renderingInfo = Allocate<VkRenderingInfo>(1u);
            state->renderingInfo->sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            state->renderingInfo->renderArea = state->renderTarget.area;
            state->renderingInfo->layerCount = state->renderTarget.layers;
            state->renderingInfo->colorAttachmentCount = colorCount;
            state->renderingInfo->pColorAttachments = colorAttachments;
            state->renderingInfo->pDepthAttachment = depthAttachment;

            for (auto i = 0u; i < PK_RHI_MAX_RENDER_TARGETS; ++i)
            {
                auto& color = colors[i];
                const auto format = i < colorCount && color.target.image.image ? color.format : VK_FORMAT_UNDEFINED;

                if (state->pipelineKey.fixed.colorFormats[i] != format)
                {
                    state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                    state->pipelineKey.fixed.colorFormats[i] = format;
                }

                if (format)
                {
                    const auto hasResolve = color.resolve.image.image != VK_NULL_HANDLE;
                    auto& attachment = colorAttachments[i];
                    attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                    attachment.pNext = nullptr;
                    attachment.imageView = m_descriptorHeap->GetRenderTargetView(color.target);
                    attachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    attachment.resolveMode = color.resolveMode;
                    attachment.resolveImageView = hasResolve ? m_descriptorHeap->GetRenderTargetView(color.resolve) : VK_NULL_HANDLE;
                    attachment.resolveImageLayout = hasResolve ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED;
                    attachment.loadOp = VulkanEnumConvert::GetLoadOp(color.loadOp);
                    attachment.storeOp = VulkanEnumConvert::GetStoreOp(color.storeOp);
                    attachment.clearValue = VulkanEnumConvert::GetClearValue(color.clearValue);
                    
                    color.loadOp = LoadOp::Load;

                    if (state->pipelineKey.fixed.sampleCountFlags != color.samples)
                    {
                        state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                        state->pipelineKey.fixed.sampleCountFlags = (uint16_t)color.samples;
                    }

                    RecordImageAccess(QueueType::Graphics, color.target, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

                    if (hasResolve)
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
                const auto hasResolve = depth.resolve.image.image != VK_NULL_HANDLE;
                const auto isStencil = VulkanEnumConvert::IsDepthStencilFormat(depthFormat);
                const auto isReadOnly = !state->dynamicState.depthStencil.depthWriteEnable && (!isStencil || state->dynamicState.depthStencil.stencilTestEnable);

                depthAttachment->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                depthAttachment->pNext = nullptr;
                depthAttachment->imageView = m_descriptorHeap->GetRenderTargetView(depth.target);
                depthAttachment->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                depthAttachment->resolveMode = depth.resolveMode;
                depthAttachment->resolveImageView = hasResolve ? m_descriptorHeap->GetRenderTargetView(depth.resolve) : VK_NULL_HANDLE;
                depthAttachment->resolveImageLayout = hasResolve ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED;
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

                if (state->pipelineKey.fixed.sampleCountFlags != depth.samples)
                {
                    state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                    state->pipelineKey.fixed.sampleCountFlags = (uint16_t)depth.samples;
                }

                RecordImageAccess(QueueType::Graphics, 
                    depth.target, 
                    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, 
                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | (!isReadOnly ? VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0u));

                if (hasResolve)
                {
                    RecordImageAccess(QueueType::Graphics, depth.resolve, VK_PIPELINE_STAGE_2_RESOLVE_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
                }
            }
        }

        if (isDirtyDynamic)
        {
            state->resolvedDynamicState = Allocate<VulkanDynamicState>(1u);
            *state->resolvedDynamicState = state->dynamicState;
        }

        cmd->renderingInfo = state->renderingInfo;
        cmd->dynamicState = state->resolvedDynamicState;
        RecordShaderCmd(QueueType::Graphics, cmd);
    }

    void VulkanGraph::RecordVertexCmd(VulkanVertexCmd* cmd)
    {
        auto state = &m_pipelineState;
        const auto shader = state->pipelineKey.shader;
        const auto& vertexLayout = shader->GetVertexLayout();
        const bool isDirtyShader = state->dirtyFlags & STATE_DIRTY_SHADER;
        const bool isDirtyBuffers = state->dirtyFlags & STATE_DIRTY_VERTEXBUFFERS;
        state->dirtyFlags &= ~STATE_DIRTY_VERTEXBUFFERS;

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
            VulkanResourceView activeBuffers[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};

            for (auto index = 0u; index < PK_RHI_MAX_VERTEX_ATTRIBUTES && state->vertexStreamLayout[index].stride; ++index)
            {
                const auto& element = state->vertexStreamLayout[index];
                auto elementIdx = 0u;

                if (state->vertexBuffers[element.stream].hash && vertexLayout.TryGetElement(element.name, &elementIdx))
                {
                    auto inputRate = VulkanEnumConvert::GetInputRate(element.inputRate);
                    --remainingAttributes;

                    if (prevElement && (prevElement->stream != element.stream || prevElement->inputRate != element.inputRate || prevElement->stride != element.stride))
                    {
                        ++streamIndex;
                    }

                    prevElement = &element;
                    activeBuffers[streamIndex] = state->vertexBuffers[element.stream];
                    reallocateBuffers |= state->resolvedVertexViews[streamIndex] != activeBuffers[streamIndex];

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

            if (streamCount < PK_RHI_MAX_VERTEX_ATTRIBUTES && state->pipelineKey.vertexStreams[streamCount].stride)
            {
                state->dirtyFlags |= STATE_DIRTY_PIPELINE;
                Memory::Memset(state->pipelineKey.vertexStreams + streamCount, 0, PK_RHI_MAX_VERTEX_ATTRIBUTES - streamCount);
            }

            if (reallocateBuffers)
            {
                state->vertexBufferCount = streamCount;
                state->resolvedVertexBuffers = Allocate<VkBindVertexBuffer3InfoKHR>(streamCount);

                for (auto i = 0u; i < streamCount; ++i)
                {
                    state->resolvedVertexViews[i] = activeBuffers[i];
                    auto& info = state->resolvedVertexBuffers[i];
                    info.sType = VK_STRUCTURE_TYPE_BIND_VERTEX_BUFFER_3_INFO_KHR;
                    info.pNext = nullptr;
                    info.setStride = VK_FALSE;
                    info.addressRange.address = activeBuffers[i].memory.address;
                    info.addressRange.size = activeBuffers[i].memory.size;
                    info.addressRange.stride = state->pipelineKey.vertexStreams[i].stride;
                    info.addressFlags = PK_VK_ADDRESS_FLAGS;
                }
            }
        }

        for (auto i = 0u; i < state->vertexBufferCount; ++i)
        {
            RecordMemoryAccess(QueueType::Graphics, state->resolvedVertexViews[i], VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT);
        }

        cmd->vertexBufferCount = state->vertexBufferCount;
        cmd->vertexBuffers = state->resolvedVertexBuffers;
        RecordRasterCmd(cmd);
    }

    void VulkanGraph::RecordIndexCmd(VulkanIndexCmd* cmd)
    {
        if (m_pipelineState.indexBuffer.hash)
        {
            RecordMemoryAccess(QueueType::Graphics, m_pipelineState.indexBuffer, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_ACCESS_2_INDEX_READ_BIT);
            cmd->indexInfo.sType = VK_STRUCTURE_TYPE_BIND_INDEX_BUFFER_3_INFO_KHR;
            cmd->indexInfo.pNext = nullptr;
            cmd->indexInfo.addressRange = m_pipelineState.indexBuffer.memory;
            cmd->indexInfo.addressFlags = PK_VK_ADDRESS_FLAGS;
            cmd->indexInfo.indexType = m_pipelineState.indexType;
        }

        RecordVertexCmd(cmd);
    }
}
