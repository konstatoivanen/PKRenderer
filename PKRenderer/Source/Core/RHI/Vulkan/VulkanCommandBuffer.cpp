#include "PrecompiledHeader.h"
#include "Core/Base/Memory.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanBuffer.h"
#include "Core/RHI/Vulkan/VulkanTexture.h"
#include "Core/RHI/Vulkan/VulkanAccelerationStructure.h"
#include "Core/RHI/Vulkan/VulkanBindSet.h"
#include "Core/RHI/Vulkan/VulkanSwapchain.h"
#include "Core/RHI/Vulkan/VulkanPipelineState.h"
#include "Core/RHI/Vulkan/VulkanTimerArena.h"
#include "VulkanCommandBuffer.h"

namespace PK
{
    FenceRef VulkanCommandBuffer::GetFenceRef() const
    {
        return FenceRef(this, [](const void* ctx, uint64_t userdata, [[maybe_unused]] uint64_t timeout)
            {
                auto cmd = static_cast<const VulkanCommandBuffer*>(ctx);
                return cmd->m_invocationIndex >= userdata;
            },
            m_invocationIndex + 1);
    }

    RHIBuffer* VulkanCommandBuffer::AcquireStagingBuffer(size_t size)
    {
        return m_stagingArena->BeginWrite(size);
    }
    
    void VulkanCommandBuffer::ReleaseStagingBuffer(RHIBuffer* buffer)
    {
        m_stagingArena->EndWrite(buffer);
    }

    void VulkanCommandBuffer::SetRenderTarget(const RenderTargetBinding* bindings, uint32_t count, const uint4& renderArea, uint32_t layers)
    {
        VulkanRenderTargetBindings state{};
        state.area = { { (int32_t)renderArea.x, (int32_t)renderArea.y}, { renderArea.z, renderArea.w } };
        state.layers = layers;
        state.colorCount = 0u;

        for (auto i = 0u; i < count; ++i)
        {
            auto binding = &bindings[i];
            auto target = binding->target->GetNativeView<VulkanBindHandle>(binding->targetRange, TextureViewMode::RTV);
            auto resolve = binding->resolve ? binding->resolve->GetNativeView<VulkanBindHandle>(binding->resolveRange, TextureViewMode::RTV) : nullptr;
            auto isDepth = VulkanEnumConvert::IsDepthFormat(target->image.format);
            auto attachment = isDepth ? &state.depth : (state.colors + state.colorCount++);
            attachment->target = target;
            attachment->resolve = resolve;
            attachment->loadOp = binding->loadOp;
            attachment->storeOp = binding->storeOp;
            attachment->clearValue = binding->clearValue;
            attachment->resolveMode = resolve ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
        }

        m_state->SetRenderTarget(state);
    }

    void VulkanCommandBuffer::SetViewPorts(const uint4* rects, uint32_t count)
    {
        VkViewport* viewports = nullptr;
        if (m_state->SetViewports(rects, count, &viewports))
        {
            vkCmdSetViewportWithCount(m_commandBuffer, count, viewports);
        }
    }

    void VulkanCommandBuffer::SetScissors(const uint4* rects, uint32_t count)
    {
        VkRect2D* scissors = nullptr;
        if (m_state->SetScissors(rects, count, &scissors))
        {
            vkCmdSetScissorWithCount(m_commandBuffer, count, scissors);
        }
    }


    void VulkanCommandBuffer::SetShader(const RHIShader* shader)
    {
        m_state->SetShader(static_cast<const VulkanShader*>(shader));
    }

    void VulkanCommandBuffer::SetVertexBuffers(const RHIBuffer** buffers, uint32_t count)
    {
        const VulkanBindHandle* pHandles[PK_RHI_MAX_VERTEX_ATTRIBUTES];

        for (auto i = 0u; i < count; ++i)
        {
            pHandles[i] = buffers[i]->GetNativeView<VulkanBindHandle>();
        }

        m_state->SetVertexBuffers(pHandles, count);
    }

    void VulkanCommandBuffer::SetVertexStreams(const VertexStreamElement* elements, uint32_t count) 
    { 
        m_state->SetVertexStreams(elements, count); 
    }

    void VulkanCommandBuffer::SetIndexBuffer(const RHIBuffer* buffer, size_t indexSize)
    {
        auto handle = buffer->GetNativeView<VulkanBindHandle>();
        m_state->SetIndexBuffer(handle, VulkanEnumConvert::GetIndexType(indexSize));
    }

    void VulkanCommandBuffer::SetShaderBindingTable(RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset, size_t stride, size_t size)
    {
        const auto address = buffer->GetDeviceAddress();
        m_state->SetShaderBindingTableAddress(group, address + offset, stride, size);
    }

    void VulkanCommandBuffer::SetStageExcludeMask(const ShaderStageFlags mask)
    {
        m_state->SetStageExcludeMask(mask);
    }

    void VulkanCommandBuffer::SetBlending(const BlendParameters& blend)
    {
        m_state->SetBlending(blend);
    }

    void VulkanCommandBuffer::SetRasterization(const RasterizationParameters& rasterization)
    {
        m_state->SetRasterization(rasterization);
    }

    void VulkanCommandBuffer::SetDepthStencil(const DepthStencilParameters& depthStencil)
    {
        m_state->SetDepthStencil(depthStencil);
    }

    void VulkanCommandBuffer::SetMultisampling(const MultisamplingParameters& multisampling)
    {
        m_state->SetMultisampling(multisampling);
    }


    void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto handleIndirect = indirectArguments->GetNativeView<VulkanBindHandle>();

        VulkanBarrierHandler::AccessRecord record{};
        record.bufferRange.offset = (uint32_t)(offset + handleIndirect->buffer.offset);
        record.bufferRange.size = drawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = PK_VK_QUEUE_FAMILY_IGNORED;
        m_barrierHandler->Record(handleIndirect->buffer.buffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawIndirect(m_commandBuffer, handleIndirect->buffer.buffer, handleIndirect->buffer.offset + offset, drawCount, stride);
    }

    void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndexedIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto handleIndirect = indirectArguments->GetNativeView<VulkanBindHandle>();

        VulkanBarrierHandler::AccessRecord record{};
        record.bufferRange.offset = (uint32_t)(offset + handleIndirect->buffer.offset);
        record.bufferRange.size = drawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = PK_VK_QUEUE_FAMILY_IGNORED;
        m_barrierHandler->Record(handleIndirect->buffer.buffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawIndexedIndirect(m_commandBuffer, handleIndirect->buffer.buffer, handleIndirect->buffer.offset + offset, drawCount, stride);
    }

    void VulkanCommandBuffer::DrawMeshTasks(const uint3& dimensions)
    {
        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawMeshTasksEXT(m_commandBuffer, dimensions.x, dimensions.y, dimensions.z);
    }

    void VulkanCommandBuffer::DrawMeshTasksIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto handleIndirect = indirectArguments->GetNativeView<VulkanBindHandle>();
        VulkanBarrierHandler::AccessRecord record{};
        record.bufferRange.offset = (uint32_t)(offset + handleIndirect->buffer.offset);
        record.bufferRange.size = drawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = PK_VK_QUEUE_FAMILY_IGNORED;
        m_barrierHandler->Record(handleIndirect->buffer.buffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawMeshTasksIndirectEXT(m_commandBuffer, handleIndirect->buffer.buffer, handleIndirect->buffer.offset + offset, drawCount, stride);
    }

    void VulkanCommandBuffer::DrawMeshTasksIndirectCount(const RHIBuffer* indirectArguments,
        size_t offset,
        const RHIBuffer* countBuffer,
        size_t countOffset,
        uint32_t maxDrawCount,
        uint32_t stride)
    {
        VulkanBarrierHandler::AccessRecord record{};
        auto handleIndirect = indirectArguments->GetNativeView<VulkanBindHandle>(); 
        record.bufferRange.offset = (uint32_t)(offset + handleIndirect->buffer.offset);
        record.bufferRange.size = maxDrawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = PK_VK_QUEUE_FAMILY_IGNORED;
        m_barrierHandler->Record(handleIndirect->buffer.buffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        auto handleCount = countBuffer->GetNativeView<VulkanBindHandle>();
        record.bufferRange.offset = (uint32_t)(countOffset + handleCount->buffer.offset);
        record.bufferRange.size = sizeof(uint32_t);
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = PK_VK_QUEUE_FAMILY_IGNORED;
        m_barrierHandler->Record(handleCount->buffer.buffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawMeshTasksIndirectCountEXT(m_commandBuffer, handleIndirect->buffer.buffer, handleIndirect->buffer.offset + offset, handleCount->buffer.buffer, handleCount->buffer.offset + countOffset, maxDrawCount, stride);
    }

    void VulkanCommandBuffer::Dispatch(const uint3& dimensions)
    {
        MarkLastCommandStage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        EndRenderPass();
        ValidatePipeline();

        const auto groupSize = m_state->GetComputeGroupSize();
        const auto groupCountX = (dimensions.x + groupSize.x - 1u) / groupSize.x;
        const auto groupCountY = (dimensions.y + groupSize.y - 1u) / groupSize.y;
        const auto groupCountZ = (dimensions.z + groupSize.z - 1u) / groupSize.z;
        vkCmdDispatch(m_commandBuffer, groupCountX, groupCountY, groupCountZ);
    }

    void VulkanCommandBuffer::DispatchRays(const uint3& dimensions)
    {
        MarkLastCommandStage(VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
        EndRenderPass();
        ValidatePipeline();
        auto addresses = m_state->GetShaderBindingTableAddresses();
        vkCmdTraceRaysKHR(m_commandBuffer,
            addresses + (uint32_t)RayTracingShaderGroup::RayGeneration,
            addresses + (uint32_t)RayTracingShaderGroup::Miss,
            addresses + (uint32_t)RayTracingShaderGroup::Hit,
            addresses + (uint32_t)RayTracingShaderGroup::Callable,
            dimensions.x,
            dimensions.y,
            dimensions.z);
    }


    void VulkanCommandBuffer::Blit(RHITexture* src, RHISwapchain* dst, FilterMode filter)
    {
        auto vkdst = static_cast<VulkanSwapchain*>(dst);
        const auto& srcHandle = src->GetNativeView<VulkanBindHandle>({}, TextureViewMode::RTV);
        const auto& dstHandle = vkdst->GetBindHandle();

        auto srcRes = src->GetResolution();
        auto dstRes = dst->GetResolution();
        auto minres = math::min(srcRes, dstRes);

        auto diff = int3(srcRes - minres);
        auto srcMin = diff / 2;
        auto srcMax = int3(srcRes) - (diff - srcMin);

        VkImageBlit blitRegion{};
        blitRegion.srcSubresource = { (uint32_t)srcHandle->image.range.aspectMask, 0u, 0u, 1u };
        blitRegion.dstSubresource = { (uint32_t)dstHandle->image.range.aspectMask, 0u, 0u, 1u };

        blitRegion.srcOffsets[0] = { srcMin.x, srcMax.y, srcMin.z };
        blitRegion.srcOffsets[1] = { srcMax.x, srcMin.y, srcMax.z };
        blitRegion.dstOffsets[0] = { 0, 0, 0 };
        blitRegion.dstOffsets[1] = { (int)dstRes.x, (int)dstRes.y, (int)dstRes.z };

        Blit(srcHandle, dstHandle, blitRegion, filter);
        ResolveSwapchainAccess(dst, false);
    }

    void VulkanCommandBuffer::Blit(RHISwapchain* src, RHIBuffer* dst)
    {
        auto vksrc = static_cast<VulkanSwapchain*>(src)->GetBindHandle();
        auto vkdst = dst->GetNativeView<VulkanBindHandle>();

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = vksrc->image.range.aspectMask;
        region.imageSubresource.mipLevel = 0u;
        region.imageSubresource.baseArrayLayer = 0u;
        region.imageSubresource.layerCount = 1u;
        region.imageOffset = { 0,0,0 };
        region.imageExtent = vksrc->image.extent;
        region.bufferOffset = vkdst->buffer.offset;

        m_state->RecordImage(m_barrierHandler, vksrc, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);

        EndRenderPass();
        ResolveBarriers();
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdCopyImageToBuffer(m_commandBuffer, vksrc->image.image, VK_IMAGE_LAYOUT_GENERAL, vkdst->buffer.buffer, 1, &region);
        ResolveSwapchainAccess(src, false);
    }

    void VulkanCommandBuffer::Blit(RHITexture* src, RHITexture* dst, const TextureViewRange& srcRange, const TextureViewRange& dstRange, FilterMode filter)
    {
        auto srcHandle = src->GetNativeView<VulkanBindHandle>(srcRange, TextureViewMode::RAW);
        auto dstHandle = dst->GetNativeView<VulkanBindHandle>(dstRange, TextureViewMode::RAW);
        auto srcLayers = math::min(srcHandle->image.range.layerCount, src->GetLayers());
        auto dstLayers = math::min(dstHandle->image.range.layerCount, dst->GetLayers());

        VkImageBlit blitRegion{};
        blitRegion.srcSubresource = { (uint32_t)srcHandle->image.range.aspectMask, srcHandle->image.range.baseMipLevel, srcHandle->image.range.baseArrayLayer, 0u };
        blitRegion.dstSubresource = { (uint32_t)srcHandle->image.range.aspectMask, dstHandle->image.range.baseMipLevel, dstHandle->image.range.baseArrayLayer, 0u };
        blitRegion.srcOffsets[1] = { (int)srcHandle->image.extent.width, (int)srcHandle->image.extent.height, (int)srcHandle->image.extent.depth };
        blitRegion.dstOffsets[1] = { (int)dstHandle->image.extent.width, (int)dstHandle->image.extent.height, (int)dstHandle->image.extent.depth };
        blitRegion.dstSubresource.layerCount = blitRegion.srcSubresource.layerCount = math::min(srcLayers, dstLayers);
        BeginDebugScope("Blit Image", PK_COLOR_RED);
        Blit(srcHandle, dstHandle, blitRegion, filter);
        EndDebugScope();
    }

    void VulkanCommandBuffer::Blit(const VulkanBindHandle* src, const VulkanBindHandle* dst, const VkImageBlit& blitRegion, FilterMode filter)
    {
        m_state->RecordImage(m_barrierHandler, src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
        m_state->RecordImage(m_barrierHandler, dst, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0u, VK_IMAGE_LAYOUT_UNDEFINED, 0u);
        m_state->RecordImage(m_barrierHandler, dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);

        EndRenderPass();
        ResolveBarriers();
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);

        auto srcBlockSize = VulkanEnumConvert::GetFormatBlockSize(src->image.format);
        auto dstBlockSize = VulkanEnumConvert::GetFormatBlockSize(dst->image.format);
        auto useCopy = srcBlockSize == dstBlockSize;
        useCopy &= blitRegion.srcOffsets[1].x - blitRegion.srcOffsets[0].x == blitRegion.dstOffsets[1].x - blitRegion.dstOffsets[0].x;
        useCopy &= blitRegion.srcOffsets[1].y - blitRegion.srcOffsets[0].y == blitRegion.dstOffsets[1].y - blitRegion.dstOffsets[0].y;
        useCopy &= blitRegion.srcOffsets[1].z - blitRegion.srcOffsets[0].z == blitRegion.dstOffsets[1].z - blitRegion.dstOffsets[0].z;
        useCopy &= blitRegion.srcSubresource.mipLevel == blitRegion.dstSubresource.mipLevel;
        useCopy &= blitRegion.srcSubresource.layerCount == blitRegion.dstSubresource.layerCount;

        if (src->image.samples > VK_SAMPLE_COUNT_1_BIT && dst->image.samples == VK_SAMPLE_COUNT_1_BIT)
        {
            VkImageResolve resolveRegion{};
            resolveRegion.srcSubresource = { (uint32_t)src->image.range.aspectMask, blitRegion.srcSubresource.mipLevel, blitRegion.srcSubresource.baseArrayLayer, blitRegion.srcSubresource.layerCount };
            resolveRegion.dstSubresource = { (uint32_t)dst->image.range.aspectMask, blitRegion.dstSubresource.mipLevel, blitRegion.dstSubresource.baseArrayLayer, blitRegion.dstSubresource.layerCount };
            resolveRegion.extent = src->image.extent;
            vkCmdResolveImage(m_commandBuffer, src->image.image, VK_IMAGE_LAYOUT_GENERAL, dst->image.image, VK_IMAGE_LAYOUT_GENERAL, 1, &resolveRegion);
        }
        else if (useCopy)
        {
            VkImageCopy copyRegion;
            copyRegion.srcSubresource = blitRegion.srcSubresource;
            copyRegion.srcOffset = blitRegion.srcOffsets[0];
            copyRegion.dstSubresource = blitRegion.dstSubresource;
            copyRegion.dstOffset = blitRegion.srcOffsets[0];
            copyRegion.extent.width = (uint32_t)blitRegion.dstOffsets[1].x - (uint32_t)blitRegion.dstOffsets[0].x;
            copyRegion.extent.height = (uint32_t)blitRegion.dstOffsets[1].y - (uint32_t)blitRegion.dstOffsets[0].y;
            copyRegion.extent.depth = (uint32_t)blitRegion.dstOffsets[1].z - (uint32_t)blitRegion.dstOffsets[0].z;
            vkCmdCopyImage(m_commandBuffer, src->image.image, VK_IMAGE_LAYOUT_GENERAL, dst->image.image, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
        }
        else
        {
            auto vkFilter = VulkanEnumConvert::GetFilterMode(filter);
            vkCmdBlitImage(m_commandBuffer, src->image.image, VK_IMAGE_LAYOUT_GENERAL, dst->image.image, VK_IMAGE_LAYOUT_GENERAL, 1, &blitRegion, vkFilter);
        }
    }


    void VulkanCommandBuffer::Clear(RHIBuffer* dst, size_t offset, size_t size, uint32_t value)
    {
        EndRenderPass();
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        auto handle = dst->GetNativeView<VulkanBindHandle>();
        vkCmdFillBuffer(m_commandBuffer, handle->buffer.buffer, handle->buffer.offset + offset, size, value);
    }

    void VulkanCommandBuffer::Clear(RHITexture* dst, const TextureViewRange& range, const TextureClearValue& value)
    {
        auto handle = dst->GetNativeView<VulkanBindHandle>(range, TextureViewMode::UAV);
        auto clearValue = VulkanEnumConvert::GetClearValue(value);

        VkClearColorValue clearColorValue{};
        memcpy(clearColorValue.uint32, &value.uint32.x, sizeof(clearColorValue.uint32));

        m_state->RecordImage(m_barrierHandler, handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_WRITE_BIT);
        ResolveBarriers();
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);

        if (VulkanEnumConvert::IsDepthFormat(handle->image.format) || VulkanEnumConvert::IsDepthStencilFormat(handle->image.format))
        {
            vkCmdClearDepthStencilImage(m_commandBuffer, handle->image.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue.depthStencil, 1, &handle->image.range);
        }
        else
        {
            vkCmdClearColorImage(m_commandBuffer, handle->image.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue.color, 1, &handle->image.range);
        }
    }


    void VulkanCommandBuffer::UpdateBuffer(RHIBuffer* dst, size_t offset, size_t size, const void* data)
    {
        EndRenderPass();
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        auto handle = dst->GetNativeView<VulkanBindHandle>();
        vkCmdUpdateBuffer(m_commandBuffer, handle->buffer.buffer, handle->buffer.offset + offset, size, data);
    }

    void VulkanCommandBuffer::CopyBuffer(RHIBuffer* dst, RHIBuffer* src, size_t srcOffset, size_t dstOffset, size_t size)
    {
        auto srcHandle = src->GetNativeView<VulkanBindHandle>();
        auto dstHandle = dst->GetNativeView<VulkanBindHandle>();
        
        VkBufferCopy copyRegion{ srcHandle->buffer.offset + srcOffset, dstHandle->buffer.offset + dstOffset, size };

        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdCopyBuffer(m_commandBuffer, srcHandle->buffer.buffer, dstHandle->buffer.buffer, 1, &copyRegion);

        VulkanBarrierHandler::AccessRecord record{};
        record.bufferRange.offset = (uint32_t)copyRegion.dstOffset;
        record.bufferRange.size = (uint32_t)copyRegion.size;
        record.stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        record.access = VK_ACCESS_TRANSFER_WRITE_BIT;
        record.queueFamily = PK_VK_QUEUE_FAMILY_IGNORED;
        m_barrierHandler->Record(dstHandle->buffer.buffer, record, PK_RHI_ACCESS_OPT_BARRIER);
    }

    void VulkanCommandBuffer::CopyToTexture(RHITexture* texture, RHIBuffer* buffer, TextureDataRegion* regions, uint32_t regionCount)
    {
        EndRenderPass();
        
        PK_DEBUG_FATAL_ASSERT(texture->GetUsage() == TextureUsage::DefaultDisk, "Texture upload is only supported for sampled | upload | readonly textures!");

        auto srcHandle = buffer->GetNativeView<VulkanBindHandle>();
        auto dstHandle = texture->GetNativeView<VulkanBindHandle>({}, TextureViewMode::RAW);
        auto resourceRange = VkImageSubresourceRange{ (uint32_t)dstHandle->image.range.aspectMask, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS, 0 };
        auto copyRegions = PK_STACK_ALLOC(VkBufferImageCopy, regionCount);

        for (auto i = 0u; i < regionCount; ++i)
        {
            auto& region = regions[i];
            copyRegions[i].bufferOffset = region.bufferOffset + srcHandle->buffer.offset;
            copyRegions[i].bufferRowLength = 0u;
            copyRegions[i].bufferImageHeight = 0u;
            copyRegions[i].imageSubresource.aspectMask = resourceRange.aspectMask;
            copyRegions[i].imageSubresource.mipLevel = region.level;
            copyRegions[i].imageSubresource.baseArrayLayer = region.layer;
            copyRegions[i].imageSubresource.layerCount = region.layers;
            copyRegions[i].imageOffset.x = region.offset.x;
            copyRegions[i].imageOffset.y = region.offset.y;
            copyRegions[i].imageOffset.z = region.offset.z;
            copyRegions[i].imageExtent.width = region.extent.x;
            copyRegions[i].imageExtent.height = region.extent.y;
            copyRegions[i].imageExtent.depth = region.extent.z;
            resourceRange.baseMipLevel = math::min(resourceRange.baseMipLevel, region.level);
            resourceRange.baseArrayLayer = math::min(resourceRange.baseArrayLayer, region.layer);
            resourceRange.levelCount = math::max(resourceRange.levelCount, region.level + 1u);
            resourceRange.layerCount = math::max(resourceRange.layerCount, region.layer + region.layers);
        }

        resourceRange.levelCount = resourceRange.levelCount - resourceRange.baseMipLevel;
        resourceRange.layerCount = resourceRange.layerCount - resourceRange.baseArrayLayer;

        VkImageMemoryBarrier imageBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = dstHandle->image.image;
        imageBarrier.subresourceRange = resourceRange;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        VulkanBarrierInfo barrier;
        barrier.imageMemoryBarrierCount = 1u;
        barrier.pImageMemoryBarriers = &imageBarrier;
        barrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        
        PipelineBarrier(barrier);
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdCopyBufferToImage(m_commandBuffer, srcHandle->buffer.buffer, dstHandle->image.image, VK_IMAGE_LAYOUT_GENERAL, regionCount, copyRegions);

        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.dstAccessMask = 0;
        barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        PipelineBarrier(barrier);
    }


    void VulkanCommandBuffer::InvalidateTexture(RHITexture* texture)
    {
        PK_DEBUG_FATAL_ASSERT(texture->GetUsage() == TextureUsage::DefaultDisk, "Texture invalidation is only supported for sampled | upload | readonly textures!");
        auto handle = texture->GetNativeView<VulkanBindHandle>({}, TextureViewMode::RAW);
        VkImageMemoryBarrier imageBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = handle->image.image;
        imageBarrier.subresourceRange = { (uint32_t)handle->image.range.aspectMask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = 0;
        VulkanBarrierInfo barrier;
        barrier.imageMemoryBarrierCount = 1u;
        barrier.pImageMemoryBarriers = &imageBarrier;
        barrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        EndRenderPass();
        PipelineBarrier(barrier);
    }

    RHIAccelerationStructureBuilder* VulkanCommandBuffer::BeginAccelerationStructureWrite(RHIAccelerationStructure* structure, uint32_t instanceLimit)
    {
        auto vkStructure = static_cast<VulkanAccelerationStructure*>(structure);
        auto writer = m_driver->arena.New<VulkanAccelerationStructureBuilder>();
        auto inputBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * instanceLimit;
        writer->structure = vkStructure;
        writer->stagingBuffer = AcquireStagingBuffer(inputBufferSize);
        writer->instances = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(writer->stagingBuffer->BeginMap(0ull, 0ull));
        writer->instanceIndices = m_driver->arena.Allocate<uint32_t>(instanceLimit);
        writer->instanceCount = 0u;
        writer->instanceLimit = instanceLimit;
        writer->topologyHash = 0ull;
        return writer;
    }

    void VulkanCommandBuffer::EndAccelerationStructureWrite(RHIAccelerationStructureBuilder* builder)
    {
        // Temp test to debug graph builder execution before actually implementing it.
        struct VulkanWriteAccelerationStructureCmd
        {
            VkQueryPool queryPool = VK_NULL_HANDLE;
            VkAccelerationStructureKHR* queryHandles = nullptr;
            uint32_t queryStart = 0u;
            uint32_t queryCount = 0u;

            VkCopyAccelerationStructureInfoKHR* copyInfos = nullptr;
            uint32_t copyCount = 0u;

            VkAccelerationStructureBuildGeometryInfoKHR* BLASBuildInfos = nullptr;
            VkAccelerationStructureBuildRangeInfoKHR** BLASRangeInfos = nullptr;
            uint32_t BLASBuildCount = 0u;

            VkAccelerationStructureBuildGeometryInfoKHR TLASBuildInfo{};
            VkAccelerationStructureBuildRangeInfoKHR TLASRangeInfo{};
            bool TLASBuild = false;
        };

        auto vkBuilder = static_cast<VulkanAccelerationStructureBuilder*>(builder);
        auto vkStructure = vkBuilder->structure;
        auto queryPool = vkStructure->queryPool.get();

        PK_DEBUG_WARNING_ASSERT(vkBuilder->instanceCount, "VulkanAccelerationStructure.EndWrite: write has 0 instances!");

        const bool hasCompactedResults = vkStructure->queryCount && queryPool->WaitResults(0ull);

        if (hasCompactedResults)
        {
            PK_LOG_RHI("Bottom Level Compaction Update: %s", vkStructure->name.c_str());
            PK_LOG_INDENT(PK_LOG_LVL_RHI);

            for (auto i = 0u; i < vkStructure->substructures.GetCount(); ++i)
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

        for (auto i = 0u; i < vkStructure->substructures.GetCount(); ++i)
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

        auto* tlasGeometry = m_driver->arena.Allocate<VkAccelerationStructureGeometryKHR>(1);
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

        VulkanWriteAccelerationStructureCmd cmd{};
        const auto substructureCount = vkStructure->substructures.GetCount();

        if (rebuildBLAS)
        {
            PK_LOG_RHI_SCOPE("Acceleration Structure Update: %s", vkStructure->name.c_str());

            FixedString128 name({ vkStructure->name.c_str(), ".StructureBuffer" });
            vkStructure->buffer = RHI::CreateBuffer(bufferSize, BufferUsage::DefaultAccelerationStructure, name.c_str());

            cmd.BLASBuildInfos = m_driver->arena.Allocate<VkAccelerationStructureBuildGeometryInfoKHR>(buildCount);
            cmd.BLASRangeInfos = m_driver->arena.Allocate<VkAccelerationStructureBuildRangeInfoKHR*>(buildCount);
            cmd.copyInfos = m_driver->arena.Allocate<VkCopyAccelerationStructureInfoKHR>(substructureCount);

            for (auto i = 0u; i < substructureCount; ++i)
            {
                auto structure = &vkStructure->substructures[i].value;
                auto newHandle = vkStructure->CreateStructure(structure, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, FixedString128({ structure->name.c_str(), ".BLAS" }));

                if (structure->handle == VK_NULL_HANDLE)
                {
                    auto* geometry = m_driver->arena.Allocate<VkAccelerationStructureGeometryKHR>(1);
                    auto* rangeInfo = m_driver->arena.Allocate<VkAccelerationStructureBuildRangeInfoKHR>(1);
                    cmd.BLASBuildInfos[cmd.BLASBuildCount] = VulkanAccelerationStructure::GetBLASBuildInfo(structure->geometry, geometry, rangeInfo);
                    cmd.BLASBuildInfos[cmd.BLASBuildCount].dstAccelerationStructure = newHandle;
                    cmd.BLASBuildInfos[cmd.BLASBuildCount].scratchData.deviceAddress = scratchBuffer->GetDeviceAddress() + structure->scratchOffset;
                    cmd.BLASRangeInfos[cmd.BLASBuildCount++] = rangeInfo;
                }
                else
                {
                    auto& copyInfo = cmd.copyInfos[cmd.copyCount++];
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

        cmd.queryPool = queryPool->pool;
        cmd.queryHandles = m_driver->arena.Allocate<VkAccelerationStructureKHR>(substructureCount);
        cmd.queryStart = vkStructure->queryCount;

        for (auto i = 0u; i < substructureCount && vkStructure->queryCount < PK_VK_MAX_AS_COMPACTIONS; ++i)
        {
            auto structure = &vkStructure->substructures[i].value;

            if (structure->handle && !structure->compactionId)
            {
                queryPool->SetFence(GetFenceRef());
                cmd.queryHandles[cmd.queryCount++] = structure->handle;
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

            cmd.TLASBuildInfo = buildInfo;
            cmd.TLASBuildInfo.dstAccelerationStructure = vkStructure->structure.handle;
            cmd.TLASBuildInfo.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress() + vkStructure->structure.scratchOffset;
            cmd.TLASRangeInfo = VkAccelerationStructureBuildRangeInfoKHR{ vkBuilder->instanceCount, 0u, 0u, 0u };
            cmd.TLASBuild = true;
        }

        if (scratchBuffer)
        {
            ReleaseStagingBuffer(scratchBuffer);
        }

        ReleaseStagingBuffer(vkBuilder->stagingBuffer);
        vkStructure->topologyHash = vkBuilder->topologyHash;
        vkStructure->instanceCount = vkBuilder->instanceCount;

        // Hypothetical execute step
        for (auto i = 0u; i < cmd.copyCount; ++i)
        {
            vkCmdCopyAccelerationStructureKHR(m_commandBuffer, &cmd.copyInfos[i]);
        }

        if (cmd.copyCount)
        {
            MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        if (cmd.BLASBuildCount)
        {
            vkCmdBuildAccelerationStructuresKHR(m_commandBuffer, cmd.BLASBuildCount, cmd.BLASBuildInfos, cmd.BLASRangeInfos);
            MarkLastCommandStage(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR);
        }

        if (cmd.BLASBuildCount || cmd.copyCount)
        {
            VkMemoryBarrier memoryBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
                VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
                VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR };

            VulkanBarrierInfo barrier;
            barrier.srcStageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
            barrier.dstStageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
            barrier.memoryBarrierCount = 1u;
            barrier.pMemoryBarriers = &memoryBarrier;
            PipelineBarrier(barrier);
        }

        if (cmd.queryCount)
        {
            vkCmdWriteAccelerationStructuresPropertiesKHR(m_commandBuffer, cmd.queryCount, cmd.queryHandles, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, cmd.queryPool, cmd.queryStart);
        }

        if (cmd.TLASBuild)
        {
            const auto* pRangeInfo = &cmd.TLASRangeInfo;
            vkCmdBuildAccelerationStructuresKHR(m_commandBuffer, 1u, &cmd.TLASBuildInfo, &pRangeInfo);
            MarkLastCommandStage(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR);
            vkStructure->lastBuildFenceRef = GetFenceRef();
        }
    }


    void VulkanCommandBuffer::BeginDebugScope(const char* name, const color& color)
    {
        if (vkCmdBeginDebugUtilsLabelEXT)
        {
            VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
            labelInfo.pNext = nullptr;
            labelInfo.pLabelName = name;
            memcpy(labelInfo.color, &color.x, sizeof(PK::color));
            vkCmdBeginDebugUtilsLabelEXT(m_commandBuffer, &labelInfo);
        }
    }

    void VulkanCommandBuffer::EndDebugScope()
    {
        if (vkCmdEndDebugUtilsLabelEXT)
        {
            vkCmdEndDebugUtilsLabelEXT(m_commandBuffer);
        }
    }

    void VulkanCommandBuffer::BeginTimer(NameID name) 
    {
        auto queryIndex = 0u;
        
        if (m_timerArena->Push(name, &queryIndex))
        {
            vkCmdWriteTimestamp(m_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_timerArena->GetQueryPool(), queryIndex);
        }
    }

    void VulkanCommandBuffer::EndTimer()
    {
        auto queryIndex = 0u;
        
        if (m_timerArena->Pop(&queryIndex))
        {
            vkCmdWriteTimestamp(m_commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_timerArena->GetQueryPool(), queryIndex);
        }
    }


    void VulkanCommandBuffer::PipelineBarrier(const VulkanBarrierInfo& barrier)
    {
        auto excludeMask = ~(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        // Memory & buffer memory barriers not allowed inside renderpasses. Barriers are not allowed inside renderpasses unless using self-dependencies.
        if (barrier.memoryBarrierCount > 0 || 
            barrier.bufferMemoryBarrierCount > 0 || 
            (barrier.srcStageMask & excludeMask) != 0 ||
            (barrier.dstStageMask & excludeMask) != 0)
        {
            EndRenderPass();
        }

        auto depedencyFlags = barrier.dependencyFlags;

        if (m_isInActiveRenderPass)
        {
            depedencyFlags |= VK_DEPENDENCY_BY_REGION_BIT;
        }

        vkCmdPipelineBarrier(m_commandBuffer,
            barrier.srcStageMask,
            barrier.dstStageMask,
            depedencyFlags,
            barrier.memoryBarrierCount,
            barrier.pMemoryBarriers,
            barrier.bufferMemoryBarrierCount,
            barrier.pBufferMemoryBarriers,
            barrier.imageMemoryBarrierCount,
            barrier.pImageMemoryBarriers);
    }

    void VulkanCommandBuffer::ResolveSwapchainAccess(RHISwapchain* swapchain, bool forceTransition)
    {
        auto vkdst = static_cast<VulkanSwapchain*>(swapchain); 
        auto signal = vkdst->ConsumeImageSignal();

        if (signal != VK_NULL_HANDLE)
        {
            m_imageSignal = signal;
        }

        if (forceTransition)
        {
            const auto& bindHandle = vkdst->GetBindHandle();
            m_state->RecordImage(m_barrierHandler, bindHandle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
            ResolveBarriers();
        }
    }

    bool VulkanCommandBuffer::ResolveBarriers()
    {
        VulkanBarrierInfo barrierInfo{};

        if (m_barrierHandler->Resolve(&barrierInfo))
        {
            PipelineBarrier(barrierInfo);
            return true;
        }

        return false;
    }

    void VulkanCommandBuffer::ValidatePipeline()
    {
        auto flags = m_state->Resolve(m_driver, m_barrierHandler, m_descriptorArena);

        if ((flags & PK_RENDER_STATE_DIRTY_RENDERTARGET) != 0)
        {
            EndRenderPass();
        }

        // Conservative barrier deployment. lets not break an active renderpass. Assume coherent read/writes.
        // Except if render target changed in which case we need to transition it.
        if (!m_isInActiveRenderPass)
        {
            ResolveBarriers();
        }

        if ((flags & PK_RENDER_STATE_DIRTY_RENDERTARGET) != 0)
        {
            auto info = m_state->GetRenderPassInfo();
            vkCmdBeginRendering(m_commandBuffer, &info);
            m_isInActiveRenderPass = true;
        }

        if ((flags & PK_RENDER_STATE_DIRTY_PIPELINE) != 0)
        {
            vkCmdBindPipeline(m_commandBuffer, m_state->GetPipelineBindPoint(), m_state->GetPipeline());
        }

        if ((flags & PK_RENDER_STATE_DIRTY_VERTEXBUFFERS) != 0)
        {
            auto vertexBufferBundle = m_state->GetVertexBufferBundle();

            if (vertexBufferBundle.count > 0)
            {
                vkCmdBindVertexBuffers(m_commandBuffer, 0, vertexBufferBundle.count, vertexBufferBundle.buffers, vertexBufferBundle.offsets);
            }
        }

        if ((flags & PK_RENDER_STATE_DIRTY_INDEXBUFFER) != 0)
        {
            VkIndexType indexType;
            auto indexBufferHandle = m_state->GetIndexBuffer(&indexType);
            vkCmdBindIndexBuffer(m_commandBuffer, indexBufferHandle->buffer.buffer, indexBufferHandle->buffer.offset, indexType);
        }

        if (m_state->HasPipeline() && (flags & PK_RENDER_STATE_DIRTY_DESCRIPTORS) != 0)
        {
            auto bufferIndex = 0u;
            const auto setOffset = m_state->GetDescriptorSetOffset();
            const auto layout = m_state->GetPipelineLayout();
            const auto bindPoint = m_state->GetPipelineBindPoint();
            vkCmdSetDescriptorBufferOffsetsEXT(m_commandBuffer, bindPoint, layout, 0, 1, &bufferIndex, &setOffset);
        }

        if (m_state->HasPipeline())
        {
            const auto resources = &m_driver->globalResources;
            const auto& constantLayout = m_state->GetPipelinePushConstantLayout();
            const auto layout = m_state->GetPipelineLayout();
            const auto stageFlags = m_state->GetPipelinePushConstantStageFlags();
            const char* data = nullptr;
            size_t dataSize = 0u;

            for (const auto& element : constantLayout)
            {
                if (resources->TryGet<char>(element.name, &data, &dataSize) && dataSize <= element.size)
                {
                    vkCmdPushConstants(m_commandBuffer, layout, stageFlags, element.offset, (uint32_t)dataSize, data);
                }
            }
        }
    }

    void VulkanCommandBuffer::EndRenderPass()
    {
        if (m_isInActiveRenderPass)
        {
            vkCmdEndRendering(m_commandBuffer);
            m_isInActiveRenderPass = false;
        }
    }

    void VulkanCommandBuffer::BeginRecord(
        const VulkanDriver* driver,
        VulkanBarrierHandler* barrierHandler,
        VulkanTimerArena* timerArena,
        VulkanStagingArena* stagingArena,
        VulkanDescriptorArena* descriptorArena,
        VulkanPipelineState* state,
        VkCommandBuffer commandBuffer,
        uint16_t queueFamily)
    {
        m_driver = driver;
        m_barrierHandler = barrierHandler;
        m_timerArena = timerArena;
        m_stagingArena = stagingArena;
        m_descriptorArena = descriptorArena;
        m_state = state;
        *m_state = VulkanPipelineState();

        m_queueTimelineIndex = ~0ull;
        m_commandBuffer = commandBuffer;
        m_queueFamily = queueFamily;
        
        m_timerArena->BeginTimeline();
        m_stagingArena->BeginRange();
        m_descriptorArena->BeginRange();

        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_ASSERT_RESULT(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));

        if (m_descriptorArena->IsValid())
        {
            VkDescriptorBufferBindingInfoEXT bindingInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT };
            bindingInfo.address = m_descriptorArena->GetDeviceAddress();
            bindingInfo.usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
            vkCmdBindDescriptorBuffersEXT(commandBuffer, 1, &bindingInfo);
        }
    }

    void VulkanCommandBuffer::EndRecord(uint64_t queueTimelineIndex)
    {
        // End possibly active render pass
        EndRenderPass();
        m_barrierHandler->ClearBarriers();
        m_timerIndex = m_timerArena->EndTimeline();
        m_stageIndex = m_stagingArena->EndRange();
        m_descriptorIndex = m_descriptorArena->EndRange();
        m_queueTimelineIndex = queueTimelineIndex;
        m_driver = nullptr;
        m_barrierHandler = nullptr;
        m_state = nullptr;

        VK_ASSERT_RESULT(vkEndCommandBuffer(m_commandBuffer));
    }

    bool VulkanCommandBuffer::Complete(uint64_t currentQueueTimelineIndex)
    {
        if (m_commandBuffer != VK_NULL_HANDLE && m_queueTimelineIndex <= currentQueueTimelineIndex)
        {
            m_timerArena->FlushTimeline(m_timerIndex);
            m_stagingArena->FreeRange(m_stageIndex);
            m_descriptorArena->FreeRange(m_descriptorIndex);
            m_timerArena = nullptr;
            m_stagingArena = nullptr;
            m_descriptorArena = nullptr;
            m_imageSignal = VK_NULL_HANDLE;
            m_commandBuffer = VK_NULL_HANDLE; 
            m_queueTimelineIndex = 0ull;
            ++m_invocationIndex;
            return true;
        }

        return false;
    }
}
