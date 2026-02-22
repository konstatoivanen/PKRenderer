#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanBuffer.h"
#include "Core/RHI/Vulkan/VulkanTexture.h"
#include "Core/RHI/Vulkan/VulkanAccelerationStructure.h"
#include "Core/RHI/Vulkan/VulkanBindSet.h"
#include "Core/RHI/Vulkan/VulkanRenderState.h"
#include "Core/RHI/Vulkan/VulkanSwapchain.h"
#include "VulkanCommandBuffer.h"

namespace PK
{
    FenceRef VulkanCommandBuffer::GetFenceRef() const
    {
        return FenceRef(this, [](const void* ctx, uint64_t userdata, [[maybe_unused]] uint64_t timeout)
            {
                auto cmd = reinterpret_cast<const VulkanCommandBuffer*>(ctx);
                return cmd->m_invocationIndex >= userdata;
            },
            m_invocationIndex + 1);
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
            auto target = static_cast<VulkanTexture*>(binding->target)->GetBindHandle(binding->targetRange, TextureBindMode::RenderTarget);
            auto resolve = binding->resolve ? static_cast<VulkanTexture*>(binding->resolve)->GetBindHandle(binding->resolveRange, TextureBindMode::RenderTarget) : nullptr;
            auto isDepth = VulkanEnumConvert::IsDepthFormat(target->image.format);
            auto attachment = isDepth ? &state.depth : (state.colors + state.colorCount++);
            attachment->target = target;
            attachment->resolve = resolve;
            attachment->loadOp = binding->loadOp;
            attachment->storeOp = binding->storeOp;
            attachment->clearValue = binding->clearValue;
            attachment->resolveMode = resolve ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
        }

        m_renderState->SetRenderTarget(state);
    }

    void VulkanCommandBuffer::SetViewPorts(const uint4* rects, uint32_t count)
    {
        VkViewport* viewports = nullptr;
        if (m_renderState->SetViewports(rects, count, &viewports))
        {
            vkCmdSetViewportWithCount(m_commandBuffer, count, viewports);
        }
    }

    void VulkanCommandBuffer::SetScissors(const uint4* rects, uint32_t count)
    {
        VkRect2D* scissors = nullptr;
        if (m_renderState->SetScissors(rects, count, &scissors))
        {
            vkCmdSetScissorWithCount(m_commandBuffer, count, scissors);
        }
    }


    void VulkanCommandBuffer::SetShader(const RHIShader* shader)
    {
        m_renderState->SetShader(static_cast<const VulkanShader*>(shader));
    }

    void VulkanCommandBuffer::SetVertexBuffers(const RHIBuffer** buffers, uint32_t count)
    {
        const VulkanBindHandle* pHandles[PK_RHI_MAX_VERTEX_ATTRIBUTES];

        for (auto i = 0u; i < count; ++i)
        {
            pHandles[i] = static_cast<const VulkanBuffer*>(buffers[i])->GetBindHandle();
        }

        m_renderState->SetVertexBuffers(pHandles, count);
    }

    void VulkanCommandBuffer::SetVertexStreams(const VertexStreamElement* elements, uint32_t count) 
    { 
        m_renderState->SetVertexStreams(elements, count); 
    }

    void VulkanCommandBuffer::SetIndexBuffer(const RHIBuffer* buffer, ElementType indexFormat)
    {
        auto handle = static_cast<const VulkanBuffer*>(buffer)->GetBindHandle();
        m_renderState->SetIndexBuffer(handle, VulkanEnumConvert::GetIndexType(indexFormat));
    }

    void VulkanCommandBuffer::SetShaderBindingTable(RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset, size_t stride, size_t size)
    {
        auto address = buffer->GetDeviceAddress();
        m_renderState->SetShaderBindingTableAddress(group, address + offset, stride, size);
    }

    void VulkanCommandBuffer::SetStageExcludeMask(const ShaderStageFlags mask)
    {
        m_renderState->SetStageExcludeMask(mask);
    }

    void VulkanCommandBuffer::SetBlending(const BlendParameters& blend)
    {
        m_renderState->SetBlending(blend);
    }

    void VulkanCommandBuffer::SetRasterization(const RasterizationParameters& rasterization)
    {
        m_renderState->SetRasterization(rasterization);
    }

    void VulkanCommandBuffer::SetDepthStencil(const DepthStencilParameters& depthStencil)
    {
        m_renderState->SetDepthStencil(depthStencil);
    }

    void VulkanCommandBuffer::SetMultisampling(const MultisamplingParameters& multisampling)
    {
        m_renderState->SetMultisampling(multisampling);
    }


    void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto vkBuffer = indirectArguments->GetNativeHandle<VkBuffer>();

        VulkanBarrierHandler::AccessRecord record{};
        record.bufferRange.offset = (uint32_t)offset;
        record.bufferRange.size = drawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = indirectArguments->IsConcurrent() ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(vkBuffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawIndirect(m_commandBuffer, vkBuffer, offset, drawCount, stride);
    }

    void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndexedIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto vkBuffer = indirectArguments->GetNativeHandle<VkBuffer>();

        VulkanBarrierHandler::AccessRecord record{};
        record.bufferRange.offset = (uint32_t)offset;
        record.bufferRange.size = drawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = indirectArguments->IsConcurrent() ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(vkBuffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawIndexedIndirect(m_commandBuffer, vkBuffer, offset, drawCount, stride);
    }

    void VulkanCommandBuffer::DrawMeshTasks(const uint3& dimensions)
    {
        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawMeshTasksEXT(m_commandBuffer, dimensions.x, dimensions.y, dimensions.z);
    }

    void VulkanCommandBuffer::DrawMeshTasksIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto vkBuffer = indirectArguments->GetNativeHandle<VkBuffer>();
        
        VulkanBarrierHandler::AccessRecord record{};
        record.bufferRange.offset = (uint32_t)offset;
        record.bufferRange.size = drawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = indirectArguments->IsConcurrent() ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(vkBuffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawMeshTasksIndirectEXT(m_commandBuffer, vkBuffer, offset, drawCount, stride);
    }

    void VulkanCommandBuffer::DrawMeshTasksIndirectCount(const RHIBuffer* indirectArguments,
        size_t offset,
        const RHIBuffer* countBuffer,
        size_t countOffset,
        uint32_t maxDrawCount,
        uint32_t stride)
    {
        VulkanBarrierHandler::AccessRecord record{};

        auto vkbufferIndirect = indirectArguments->GetNativeHandle<VkBuffer>();
        record.bufferRange.offset = (uint32_t)offset;
        record.bufferRange.size = maxDrawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = indirectArguments->IsConcurrent() ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(vkbufferIndirect, record, PK_RHI_ACCESS_OPT_BARRIER);

        auto vkbufferCount = countBuffer->GetNativeHandle<VkBuffer>();
        record.bufferRange.offset = (uint32_t)countOffset;
        record.bufferRange.size = sizeof(uint32_t);
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = indirectArguments->IsConcurrent() ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(vkbufferCount, record, PK_RHI_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawMeshTasksIndirectCountEXT(m_commandBuffer, vkbufferIndirect, offset, vkbufferCount, countOffset, maxDrawCount, stride);
    }

    void VulkanCommandBuffer::Dispatch(const uint3& dimensions)
    {
        MarkLastCommandStage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        EndRenderPass();
        ValidatePipeline();

        const auto groupSize = m_renderState->GetComputeGroupSize();
        const auto groupCountX = (uint32_t)ceilf(dimensions.x / (float)groupSize.x);
        const auto groupCountY = (uint32_t)ceilf(dimensions.y / (float)groupSize.y);
        const auto groupCountZ = (uint32_t)ceilf(dimensions.z / (float)groupSize.z);
        vkCmdDispatch(m_commandBuffer, groupCountX, groupCountY, groupCountZ);
    }

    void VulkanCommandBuffer::DispatchRays(const uint3& dimensions)
    {
        MarkLastCommandStage(VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
        EndRenderPass();
        ValidatePipeline();
        auto addresses = m_renderState->GetShaderBindingTableAddresses();
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
        auto vksrc = static_cast<VulkanTexture*>(src);
        auto vkdst = static_cast<VulkanSwapchain*>(dst);
        const auto& srcHandle = vksrc->GetBindHandle(TextureBindMode::RenderTarget);
        const auto& dstHandle = vkdst->GetBindHandle();

        auto srcRes = src->GetResolution();
        auto dstRes = dst->GetResolution();
        auto minres = glm::min(srcRes, dstRes);

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
        auto vkdst = dst->GetNativeHandle<VkBuffer>();

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = vksrc->image.range.aspectMask;
        region.imageSubresource.mipLevel = 0u;
        region.imageSubresource.baseArrayLayer = 0u;
        region.imageSubresource.layerCount = 1u;
        region.imageOffset = { 0,0,0 };
        region.imageExtent = vksrc->image.extent;

        m_renderState->RecordImage(vksrc, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        EndRenderPass();
        ResolveBarriers();
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdCopyImageToBuffer(m_commandBuffer, vksrc->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkdst, 1, &region);
        ResolveSwapchainAccess(src, false);
    }

    void VulkanCommandBuffer::Blit(RHITexture* src, RHITexture* dst, const TextureViewRange& srcRange, const TextureViewRange& dstRange, FilterMode filter)
    {
        VulkanBindHandle srcHandle;
        VulkanBindHandle dstHandle;
        static_cast<VulkanTexture*>(src)->FillBindHandle(&srcHandle, srcRange, TextureBindMode::RenderTarget);
        static_cast<VulkanTexture*>(dst)->FillBindHandle(&dstHandle, dstRange, TextureBindMode::RenderTarget);
        auto srcLayers = glm::min(srcHandle.image.range.layerCount, src->GetLayers());
        auto dstLayers = glm::min(dstHandle.image.range.layerCount, dst->GetLayers());

        VkImageBlit blitRegion{};
        blitRegion.srcSubresource = { (uint32_t)srcHandle.image.range.aspectMask, srcHandle.image.range.baseMipLevel, srcHandle.image.range.baseArrayLayer, 0u };
        blitRegion.dstSubresource = { (uint32_t)srcHandle.image.range.aspectMask, dstHandle.image.range.baseMipLevel, dstHandle.image.range.baseArrayLayer, 0u };
        blitRegion.srcOffsets[1] = { (int)srcHandle.image.extent.width, (int)srcHandle.image.extent.height, (int)srcHandle.image.extent.depth };
        blitRegion.dstOffsets[1] = { (int)dstHandle.image.extent.width, (int)dstHandle.image.extent.height, (int)dstHandle.image.extent.depth };
        blitRegion.dstSubresource.layerCount = blitRegion.srcSubresource.layerCount = glm::min(srcLayers, dstLayers);
        BeginDebugScope("Blit Image", PK_COLOR_RED);
        Blit(&srcHandle, &dstHandle, blitRegion, filter);
        EndDebugScope();
    }

    void VulkanCommandBuffer::Blit(const VulkanBindHandle* src, const VulkanBindHandle* dst, const VkImageBlit& blitRegion, FilterMode filter)
    {
        m_renderState->RecordImage(src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        m_renderState->RecordImage(dst, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0u, VK_IMAGE_LAYOUT_UNDEFINED, 0u);
        m_renderState->RecordImage(dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        EndRenderPass();
        ResolveBarriers();
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);

        auto srcBlockSize = VulkanEnumConvert::GetFormatBlockSize(src->image.format);
        auto dstBlockSize = VulkanEnumConvert::GetFormatBlockSize(dst->image.format);
        auto useCopy = srcBlockSize == dstBlockSize;
        useCopy &= blitRegion.srcOffsets[0].x == blitRegion.dstOffsets[0].x;
        useCopy &= blitRegion.srcOffsets[0].y == blitRegion.dstOffsets[0].y;
        useCopy &= blitRegion.srcOffsets[0].z == blitRegion.dstOffsets[0].z;
        useCopy &= blitRegion.dstOffsets[1].x == blitRegion.dstOffsets[1].x;
        useCopy &= blitRegion.srcOffsets[1].y == blitRegion.dstOffsets[1].y;
        useCopy &= blitRegion.srcOffsets[1].z == blitRegion.dstOffsets[1].z;
        useCopy &= blitRegion.srcSubresource.mipLevel == blitRegion.dstSubresource.mipLevel;
        useCopy &= blitRegion.srcSubresource.layerCount == blitRegion.dstSubresource.layerCount;

        if (src->image.samples > VK_SAMPLE_COUNT_1_BIT && dst->image.samples == VK_SAMPLE_COUNT_1_BIT)
        {
            VkImageResolve resolveRegion{};
            resolveRegion.srcSubresource = { (uint32_t)src->image.range.aspectMask, blitRegion.srcSubresource.mipLevel, blitRegion.srcSubresource.baseArrayLayer, blitRegion.srcSubresource.layerCount };
            resolveRegion.dstSubresource = { (uint32_t)dst->image.range.aspectMask, blitRegion.dstSubresource.mipLevel, blitRegion.dstSubresource.baseArrayLayer, blitRegion.dstSubresource.layerCount };
            resolveRegion.extent = src->image.extent;
            vkCmdResolveImage(m_commandBuffer, src->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &resolveRegion);
        }
        else if (useCopy)
        {
            VkImageCopy copyRegion;
            copyRegion.srcSubresource = blitRegion.srcSubresource;
            copyRegion.srcOffset = blitRegion.srcOffsets[0];
            copyRegion.dstSubresource = blitRegion.dstSubresource;
            copyRegion.dstOffset = blitRegion.srcOffsets[0];
            copyRegion.extent = { (uint32_t)blitRegion.dstOffsets[1].x, (uint32_t)blitRegion.dstOffsets[1].y, (uint32_t)blitRegion.dstOffsets[1].z };
            vkCmdCopyImage(m_commandBuffer, src->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
        }
        else
        {
            auto vkFilter = VulkanEnumConvert::GetFilterMode(filter);
            vkCmdBlitImage(m_commandBuffer, src->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, vkFilter);
        }
    }


    void VulkanCommandBuffer::Clear(RHIBuffer* dst, size_t offset, size_t size, uint32_t value)
    {
        EndRenderPass();
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdFillBuffer(m_commandBuffer, dst->GetNativeHandle<VkBuffer>(), offset, size, value);
    }

    void VulkanCommandBuffer::Clear(RHITexture* dst, const TextureViewRange& range, const TextureClearValue& value)
    {
        auto handle = static_cast<VulkanTexture*>(dst)->GetBindHandle(range, TextureBindMode::Image);
        auto clearValue = VulkanEnumConvert::GetClearValue(value);

        VkClearColorValue clearColorValue{};
        memcpy(clearColorValue.uint32, glm::value_ptr(value.uint32), sizeof(clearColorValue.uint32));

        m_renderState->RecordImage(handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);
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
        vkCmdUpdateBuffer(m_commandBuffer, dst->GetNativeHandle<VkBuffer>(), offset, size, data);
    }

    void VulkanCommandBuffer::CopyBuffer(RHIBuffer* dst, RHIBuffer* src, size_t srcOffset, size_t dstOffset, size_t size)
    {
        VkBufferCopy copyRegion{ srcOffset, dstOffset, size };
        auto vksrcBuffer = src->GetNativeHandle<VkBuffer>();
        auto vkdstBuffer = dst->GetNativeHandle<VkBuffer>();

        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdCopyBuffer(m_commandBuffer, vksrcBuffer, vkdstBuffer, 1, &copyRegion);

        VulkanBarrierHandler::AccessRecord record{};
        record.bufferRange.offset = (uint32_t)copyRegion.dstOffset;
        record.bufferRange.size = (uint32_t)copyRegion.size;
        record.stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        record.access = VK_ACCESS_TRANSFER_WRITE_BIT;
        record.queueFamily = dst->IsConcurrent() ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(vkdstBuffer, record, PK_RHI_ACCESS_OPT_BARRIER);
    }

    void* VulkanCommandBuffer::BeginBufferWrite(RHIBuffer* buffer, size_t offset, size_t size)
    {
        return static_cast<VulkanBuffer*>(buffer)->BeginStagedWrite(offset, size);
    }

    void VulkanCommandBuffer::EndBufferWrite(RHIBuffer* buffer)
    {
        VkBufferCopy copyRegion;
        RHIBuffer* src;
        RHIBuffer* dst;
        static_cast<VulkanBuffer*>(buffer)->EndStagedWrite(&dst, &src, &copyRegion, GetFenceRef());
        CopyBuffer(dst, src, copyRegion.srcOffset, copyRegion.dstOffset, copyRegion.size);
    }


    void VulkanCommandBuffer::CopyToTexture(RHITexture* texture, RHIBuffer* buffer, TextureDataRegion* regions, uint32_t regionCount)
    {
        PK_DEBUG_THROW_ASSERT(texture->GetUsage() == TextureUsage::DefaultDisk, "Texture upload is only supported for sampled | upload | readonly textures!");

        auto vkTexture = static_cast<VulkanTexture*>(texture);
        auto layout = vkTexture->GetImageLayout();
        auto vkBuffer = buffer->GetNativeHandle<VkBuffer>();
        auto vkImage = texture->GetNativeHandle<VkImage>();

        auto resourceRange = VkImageSubresourceRange{ (uint32_t)vkTexture->GetAspectFlags(), VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS, 0 };
        auto copyRegions = PK_STACK_ALLOC(VkBufferImageCopy, regionCount);

        for (auto i = 0u; i < regionCount; ++i)
        {
            auto& region = regions[i];
            copyRegions[i].bufferOffset = region.bufferOffset;
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
            resourceRange.baseMipLevel = glm::min(resourceRange.baseMipLevel, region.level);
            resourceRange.baseArrayLayer = glm::min(resourceRange.baseArrayLayer, region.layer);
            resourceRange.levelCount = glm::max(resourceRange.levelCount, region.level + 1u);
            resourceRange.layerCount = glm::max(resourceRange.layerCount, region.layer + region.layers);
        }

        resourceRange.levelCount = resourceRange.levelCount - resourceRange.baseMipLevel;
        resourceRange.layerCount = resourceRange.layerCount - resourceRange.baseArrayLayer;

        TransitionImageLayout(vkImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, resourceRange);
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdCopyBufferToImage(m_commandBuffer, vkBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regionCount, copyRegions);
        TransitionImageLayout(vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, resourceRange);
    }

    void VulkanCommandBuffer::CopyToTexture(RHITexture* texture, const void* data, size_t size, TextureDataRegion* regions, uint32_t regionCount)
    {
        auto stage = m_renderState->GetServices()->stagingBufferCache->Acquire(size, false, nullptr);

        auto pMapped = stage->BeginMap(0ull, 0ull);
        memcpy(pMapped, data, size);
        stage->EndMap(0ull, size);
        
        CopyToTexture(texture, stage, regions, regionCount);
        m_renderState->GetServices()->stagingBufferCache->Release(stage, GetFenceRef());
    }


    void VulkanCommandBuffer::BeginDebugScope(const char* name, const color& color)
    {
        if (vkCmdBeginDebugUtilsLabelEXT)
        {
            VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
            labelInfo.pNext = nullptr;
            labelInfo.pLabelName = name;
            memcpy(labelInfo.color, glm::value_ptr(color), sizeof(PK::color));
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


    void VulkanCommandBuffer::BuildAccelerationStructures(uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
    {
        MarkLastCommandStage(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR);
        vkCmdBuildAccelerationStructuresKHR(m_commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
    }

    void VulkanCommandBuffer::CopyAccelerationStructure(const VkCopyAccelerationStructureInfoKHR* pInfo)
    {
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdCopyAccelerationStructureKHR(m_commandBuffer, pInfo);
    }

    int32_t VulkanCommandBuffer::QueryAccelerationStructureCompactSize(const VulkanRawAccelerationStructure* structure, VulkanQueryPool* pool)
    {
        PK_DEBUG_THROW_ASSERT(pool->type == VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, "Invalid query pool type");
        auto queryIndex = pool->AddQuery(GetFenceRef());

        if (queryIndex != -1)
        {
            vkCmdWriteAccelerationStructuresPropertiesKHR(m_commandBuffer, 1u, &structure->structure, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, pool->pool, (uint32_t)queryIndex);
        }

        return queryIndex;
    }

    void VulkanCommandBuffer::TransitionImageLayout(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, const VkImageSubresourceRange& range)
    {
        if (srcLayout == dstLayout)
        {
            return;
        }

        VkImageMemoryBarrier imageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        imageBarrier.oldLayout = srcLayout;
        imageBarrier.newLayout = dstLayout;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = image;
        imageBarrier.subresourceRange = range;

        VulkanBarrierInfo barrier;
        barrier.imageMemoryBarrierCount = 1u;
        barrier.pImageMemoryBarriers = &imageBarrier;

        switch (dstLayout)
        {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                imageBarrier.srcAccessMask = 0;
                imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                imageBarrier.srcAccessMask = 0;
                imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_GENERAL:
                imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageBarrier.dstAccessMask = 0u;
                barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                barrier.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                break;
            default:
                PK_THROW_ERROR("Unsupported layout transition!");
        }

        EndRenderPass();
        PipelineBarrier(barrier);
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
            m_renderState->RecordImage(bindHandle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE);
            ResolveBarriers();
        }
    }

    bool VulkanCommandBuffer::ResolveBarriers()
    {
        VulkanBarrierInfo barrierInfo{};

        if (m_renderState->GetServices()->barrierHandler->Resolve(&barrierInfo))
        {
            PipelineBarrier(barrierInfo);
            return true;
        }

        return false;
    }

    void VulkanCommandBuffer::ValidatePipeline()
    {
        auto flags = m_renderState->ValidatePipeline(GetFenceRef());

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
            auto info = m_renderState->GetRenderPassInfo();
            vkCmdBeginRendering(m_commandBuffer, &info);
            m_isInActiveRenderPass = true;
        }

        if ((flags & PK_RENDER_STATE_DIRTY_PIPELINE) != 0)
        {
            vkCmdBindPipeline(m_commandBuffer, m_renderState->GetPipelineBindPoint(), m_renderState->GetPipeline());
        }

        if ((flags & PK_RENDER_STATE_DIRTY_VERTEXBUFFERS) != 0)
        {
            auto vertexBufferBundle = m_renderState->GetVertexBufferBundle();

            if (vertexBufferBundle.count > 0)
            {
                vkCmdBindVertexBuffers(m_commandBuffer, 0, vertexBufferBundle.count, vertexBufferBundle.buffers, vertexBufferBundle.offsets);
            }
        }

        if ((flags & PK_RENDER_STATE_DIRTY_INDEXBUFFER) != 0)
        {
            VkIndexType indexType;
            auto indexBufferHandle = m_renderState->GetIndexBuffer(&indexType);
            vkCmdBindIndexBuffer(m_commandBuffer, indexBufferHandle->buffer.buffer, indexBufferHandle->buffer.offset, indexType);
        }

        if (m_renderState->HasPipeline() && (flags & PK_RENDER_STATE_DIRTY_DESCRIPTORS) != 0)
        {
            const auto descriptorSet = m_renderState->GetDescriptorSet();
            const auto layout = m_renderState->GetPipelineLayout();
            const auto bindPoint = m_renderState->GetPipelineBindPoint();
            vkCmdBindDescriptorSets(m_commandBuffer, bindPoint, layout, 0u, 1u, &descriptorSet, 0, nullptr);
        }

        if (m_renderState->HasPipeline())
        {
            const auto resources = m_renderState->GetServices()->globalResources;
            const auto& constantLayout = m_renderState->GetPipelinePushConstantLayout();
            const auto layout = m_renderState->GetPipelineLayout();
            const auto stageFlags = m_renderState->GetPipelinePushConstantStageFlags();
            const char* data = nullptr;
            size_t dataSize = 0u;

            for (auto& element : constantLayout)
            {
                if (resources->TryGet<char>(element->name, data, &dataSize) && dataSize <= element->size)
                {
                    vkCmdPushConstants(m_commandBuffer, layout, stageFlags, element->offset, (uint32_t)dataSize, data);
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

    void VulkanCommandBuffer::BeginRecord(VkCommandBuffer commandBuffer, VkFence fence, uint16_t queueFamily, VulkanRenderState* renderState)
    {
        m_commandBuffer = commandBuffer;
        m_fence = fence;
        m_queueFamily = queueFamily;
        m_renderState = renderState;
        m_renderState->Reset();

        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_ASSERT_RESULT(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
    }

    void VulkanCommandBuffer::EndRecord()
    {
        // End possibly active render pass
        EndRenderPass();
        m_renderState->GetServices()->barrierHandler->ClearBarriers();
        VK_ASSERT_RESULT(vkEndCommandBuffer(m_commandBuffer));
        m_renderState = nullptr;
    }

    void VulkanCommandBuffer::FinishExecution()
    {
        m_imageSignal = VK_NULL_HANDLE;
        m_commandBuffer = VK_NULL_HANDLE; 
        m_fence = VK_NULL_HANDLE;
        ++m_invocationIndex;
    }
}
