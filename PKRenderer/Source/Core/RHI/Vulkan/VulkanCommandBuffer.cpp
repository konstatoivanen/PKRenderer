#include "PrecompiledHeader.h"
#include "Core/Utilities/Handle.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanBuffer.h"
#include "Core/RHI/Vulkan/VulkanTexture.h"
#include "Core/RHI/Vulkan/VulkanAccelerationStructure.h"
#include "Core/RHI/Vulkan/VulkanBindArray.h"
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
            auto target = binding->target->GetNative<VulkanTexture>()->GetBindHandle(binding->targetRange, TextureBindMode::RenderTarget);
            auto resolve = binding->resolve ? binding->resolve->GetNative<VulkanTexture>()->GetBindHandle(binding->resolveRange, TextureBindMode::RenderTarget) : nullptr;
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
        m_renderState->SetShader(shader->GetNative<VulkanShader>());
    }

    void VulkanCommandBuffer::SetVertexBuffers(const RHIBuffer** buffers, uint32_t count)
    {
        const VulkanBindHandle* pHandles[PK_RHI_MAX_VERTEX_ATTRIBUTES];

        for (auto i = 0u; i < count; ++i)
        {
            pHandles[i] = buffers[i]->GetNative<VulkanBuffer>()->GetBindHandle();
        }

        m_renderState->SetVertexBuffers(pHandles, count);
    }

    void VulkanCommandBuffer::SetVertexStreams(const VertexStreamElement* elements, uint32_t count) 
    { 
        m_renderState->SetVertexStreams(elements, count); 
    }

    void VulkanCommandBuffer::SetIndexBuffer(const RHIBuffer* buffer, ElementType indexFormat)
    {
        auto handle = buffer->GetNative<VulkanBuffer>()->GetBindHandle();
        m_renderState->SetIndexBuffer(handle, VulkanEnumConvert::GetIndexType(indexFormat));
    }

    void VulkanCommandBuffer::SetShaderBindingTable(RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset, size_t stride, size_t size)
    {
        auto address = buffer->GetNative<VulkanBuffer>()->GetRaw()->deviceAddress;
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
        static VulkanBarrierHandler::AccessRecord record{};

        auto vkbuffer = indirectArguments->GetNative<VulkanBuffer>()->GetRaw();
        record.bufferRange.offset = (uint32_t)offset;
        record.bufferRange.size = drawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = indirectArguments->IsConcurrent() ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(vkbuffer->buffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawIndirect(m_commandBuffer, vkbuffer->buffer, offset, drawCount, stride);
    }

    void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndexedIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        static VulkanBarrierHandler::AccessRecord record{};

        auto vkbuffer = indirectArguments->GetNative<VulkanBuffer>()->GetRaw();
        record.bufferRange.offset = (uint32_t)offset;
        record.bufferRange.size = drawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = indirectArguments->IsConcurrent() ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(vkbuffer->buffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawIndexedIndirect(m_commandBuffer, vkbuffer->buffer, offset, drawCount, stride);
    }

    void VulkanCommandBuffer::DrawMeshTasks(const uint3& dimensions)
    {
        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawMeshTasksEXT(m_commandBuffer, dimensions.x, dimensions.y, dimensions.z);
    }

    void VulkanCommandBuffer::DrawMeshTasksIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        static VulkanBarrierHandler::AccessRecord record{};

        auto vkbuffer = indirectArguments->GetNative<VulkanBuffer>()->GetRaw();
        record.bufferRange.offset = (uint32_t)offset;
        record.bufferRange.size = drawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = indirectArguments->IsConcurrent() ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(vkbuffer->buffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawMeshTasksIndirectEXT(m_commandBuffer, vkbuffer->buffer, offset, drawCount, stride);
    }

    void VulkanCommandBuffer::DrawMeshTasksIndirectCount(const RHIBuffer* indirectArguments,
        size_t offset,
        const RHIBuffer* countBuffer,
        size_t countOffset,
        uint32_t maxDrawCount,
        uint32_t stride)
    {
        static VulkanBarrierHandler::AccessRecord record{};

        auto vkbufferIndirect = indirectArguments->GetNative<VulkanBuffer>()->GetRaw();
        record.bufferRange.offset = (uint32_t)offset;
        record.bufferRange.size = maxDrawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = indirectArguments->IsConcurrent() ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(vkbufferIndirect->buffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        auto vkbufferCount = countBuffer->GetNative<VulkanBuffer>()->GetRaw();
        record.bufferRange.offset = (uint32_t)countOffset;
        record.bufferRange.size = sizeof(uint32_t);
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = indirectArguments->IsConcurrent() ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(vkbufferCount->buffer, record, PK_RHI_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        MarkLastCommandStage(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        vkCmdDrawMeshTasksIndirectCountEXT(m_commandBuffer, vkbufferIndirect->buffer, offset, vkbufferCount->buffer, countOffset, maxDrawCount, stride);
    }

    void VulkanCommandBuffer::Dispatch(const uint3& dimensions)
    {
        BeginDebugScope(m_renderState->GetShaderName(), PK_COLOR_MAGENTA);
        MarkLastCommandStage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        EndRenderPass();
        ValidatePipeline();

        uint3 groupSize = m_renderState->GetComputeGroupSize();
        uint32_t groupCountX = (uint32_t)ceilf(dimensions.x / (float)groupSize.x);
        uint32_t groupCountY = (uint32_t)ceilf(dimensions.y / (float)groupSize.y);
        uint32_t groupCountZ = (uint32_t)ceilf(dimensions.z / (float)groupSize.z);

        // Add debug scopes to compute calls as they're not otherwise visible in NSight timeline
        vkCmdDispatch(m_commandBuffer, groupCountX, groupCountY, groupCountZ);
        EndDebugScope();
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
        auto vksrc = src->GetNative<VulkanTexture>();
        auto vkdst = dst->GetNative<VulkanSwapchain>();
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
        m_renderState->RecordImage(dstHandle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
    }

    void VulkanCommandBuffer::Blit(RHISwapchain* src, RHIBuffer* dst)
    {
        auto vksrc = src->GetNative<VulkanSwapchain>()->GetBindHandle();
        auto vkbuff = dst->GetNative<VulkanBuffer>();

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
        vkCmdCopyImageToBuffer(m_commandBuffer, vksrc->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkbuff->GetRaw()->buffer, 1, &region);
    }

    void VulkanCommandBuffer::Blit(RHITexture* src, RHITexture* dst, const TextureViewRange& srcRange, const TextureViewRange& dstRange, FilterMode filter)
    {
        static VulkanBindHandle srcHandle;
        static VulkanBindHandle dstHandle;
        src->GetNative<VulkanTexture>()->FillBindHandle(&srcHandle, srcRange, TextureBindMode::RenderTarget);
        dst->GetNative<VulkanTexture>()->FillBindHandle(&dstHandle, dstRange, TextureBindMode::RenderTarget);
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
        vkCmdFillBuffer(m_commandBuffer, dst->GetNative<VulkanBuffer>()->GetRaw()->buffer, offset, size, value);
    }


    void VulkanCommandBuffer::Clear(RHITexture* dst, const TextureViewRange& range, const TextureClearValue& value)
    {
        auto vktex = dst->GetNative<VulkanTexture>();
        auto handle = vktex->GetBindHandle(range, TextureBindMode::Image);
        auto clearValue = VulkanEnumConvert::GetClearValue(value);

        VkClearColorValue clearColorValue{};
        memcpy(clearColorValue.uint32, glm::value_ptr(value.uint32), sizeof(clearColorValue.uint32));

        m_renderState->RecordImage(handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);
        ResolveBarriers();
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);

        if (VulkanEnumConvert::IsDepthFormat(handle->image.format) || VulkanEnumConvert::IsDepthStencilFormat(handle->image.format))
        {
            vkCmdClearDepthStencilImage(m_commandBuffer, vktex->GetRaw()->image, VK_IMAGE_LAYOUT_GENERAL, &clearValue.depthStencil, 1, &handle->image.range);
        }
        else
        {
            vkCmdClearColorImage(m_commandBuffer, vktex->GetRaw()->image, VK_IMAGE_LAYOUT_GENERAL, &clearValue.color, 1, &handle->image.range);
        }
    }

    void VulkanCommandBuffer::UpdateBuffer(RHIBuffer* dst, size_t offset, size_t size, void* data)
    {
        EndRenderPass();
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdUpdateBuffer(m_commandBuffer, dst->GetNative<VulkanBuffer>()->GetRaw()->buffer, offset, size, data);
    }

    void* VulkanCommandBuffer::BeginBufferWrite(RHIBuffer* buffer, size_t offset, size_t size)
    {
        return buffer->GetNative<VulkanBuffer>()->BeginWrite(offset, size);
    }

    void VulkanCommandBuffer::EndBufferWrite(RHIBuffer* buffer)
    {
        VkBufferCopy copyRegion;
        VkBuffer srcBuffer, dstBuffer;

        auto vkBuffer = buffer->GetNative<VulkanBuffer>();
        vkBuffer->EndWrite(&srcBuffer, &dstBuffer, &copyRegion, GetFenceRef());

        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdCopyBuffer(m_commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        static VulkanBarrierHandler::AccessRecord record{};
        record.bufferRange.offset = (uint32_t)copyRegion.dstOffset;
        record.bufferRange.size = (uint32_t)copyRegion.size;
        record.stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        record.access = VK_ACCESS_TRANSFER_WRITE_BIT;
        record.queueFamily = buffer->IsConcurrent() ? (uint16_t)VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(dstBuffer, record, PK_RHI_ACCESS_OPT_BARRIER);
    }

    void VulkanCommandBuffer::UploadTexture(RHITexture* texture, const void* data, size_t size, TextureUploadRange* ranges, uint32_t rangeCount)
    {
        PK_THROW_ASSERT(texture->GetUsage() == TextureUsage::DefaultDisk, "Texture upload is only supported for sampled | upload | readonly textures!");

        auto vkTexture = texture->GetNative<VulkanTexture>();
        auto layout = vkTexture->GetImageLayout();
        auto range = VkImageSubresourceRange{ (uint32_t)vkTexture->GetAspectFlags(), 0, texture->GetLevels(), 0, texture->GetLayers() };
        auto stage = m_renderState->GetServices()->stagingBufferCache->Acquire(size, false, nullptr);

        // Assume a common case of max 5 mips. avoids redundant mallocs.
        MemoryBlock<VkBufferImageCopy, 5ull> regions;
        regions.Validate(rangeCount);

        for (auto i = 0u; i < rangeCount; ++i)
        {
            auto& range = ranges[i];
            auto& region = regions[i];

            region.bufferOffset = range.bufferOffset;
            // Tightly packed.
            region.bufferRowLength = 0u;
            region.bufferImageHeight = 0u;
            region.imageSubresource.aspectMask = vkTexture->GetAspectFlags();
            region.imageSubresource.mipLevel = range.level;
            region.imageSubresource.baseArrayLayer = range.layer;
            region.imageSubresource.layerCount = range.layers;
            region.imageOffset.x = range.offset.x;
            region.imageOffset.y = range.offset.y;
            region.imageOffset.z = range.offset.z;
            region.imageExtent.width = range.extent.x;
            region.imageExtent.height = range.extent.y;
            region.imageExtent.depth = range.extent.z;
        }

        stage->SetData(data, size);
        TransitionImageLayout(vkTexture->GetRaw()->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdCopyBufferToImage(m_commandBuffer, stage->buffer, vkTexture->GetRaw()->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, rangeCount, regions.GetData());
        TransitionImageLayout(vkTexture->GetRaw()->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, range);
        m_renderState->GetServices()->stagingBufferCache->Release(stage, GetFenceRef());
    }

    void VulkanCommandBuffer::UploadTexture(RHITexture* texture, const void* data, size_t size, uint32_t level, uint32_t layer)
    {
        PK_THROW_ASSERT(texture->GetUsage() == TextureUsage::DefaultDisk, "Texture upload is only supported for sampled | upload | readonly textures!");

        auto vkTexture = texture->GetNative<VulkanTexture>();
        auto extent = vkTexture->GetRaw()->extent;
        auto image = vkTexture->GetRaw()->image;
        auto layout = vkTexture->GetImageLayout();
        auto range = VkImageSubresourceRange{ (uint32_t)vkTexture->GetAspectFlags(), level, 1, layer, 1 };
        auto stage = m_renderState->GetServices()->stagingBufferCache->Acquire(size, false, nullptr);

        VkBufferImageCopy copyRegion{};
        copyRegion.imageSubresource.aspectMask = vkTexture->GetAspectFlags();
        copyRegion.imageSubresource.mipLevel = level;
        copyRegion.imageSubresource.baseArrayLayer = layer;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = extent;
        copyRegion.imageExtent.width >>= level;
        copyRegion.imageExtent.height >>= level;
        copyRegion.imageExtent.depth >>= level;

        stage->SetData(data, size);
        TransitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
        MarkLastCommandStage(VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdCopyBufferToImage(m_commandBuffer, stage->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copyRegion);
        TransitionImageLayout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, range);
        m_renderState->GetServices()->stagingBufferCache->Release(stage, GetFenceRef());
    }

    void VulkanCommandBuffer::BeginDebugScope(const char* name, const color& color)
    {
        VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
        labelInfo.pNext = nullptr;
        labelInfo.pLabelName = name;
        memcpy(labelInfo.color, glm::value_ptr(color), sizeof(PK::color));
        vkCmdBeginDebugUtilsLabelEXT(m_commandBuffer, &labelInfo);
    }

    void VulkanCommandBuffer::EndDebugScope()
    {
        vkCmdEndDebugUtilsLabelEXT(m_commandBuffer);
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
        PK_THROW_ASSERT(pool->type == VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, "Invalid query pool type");
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

    void VulkanCommandBuffer::ValidateSwapchainPresent(RHISwapchain* swapchain)
    {
        auto vkdst = swapchain->GetNative<VulkanSwapchain>();
        const auto& bindHandle = vkdst->GetBindHandle();
        m_renderState->RecordImage(bindHandle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE);
        ResolveBarriers();
    }

    bool VulkanCommandBuffer::ResolveBarriers()
    {
        static VulkanBarrierInfo barrierInfo{};

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

        // Conservative barrier deployment. lets not break an active renderpass. Assume coherent read/writes.
        if (!m_isInActiveRenderPass || (flags & PK_RENDER_STATE_DIRTY_RENDERTARGET) != 0)
        {
            ResolveBarriers();
        }

        if ((flags & PK_RENDER_STATE_DIRTY_RENDERTARGET) != 0)
        {
            EndRenderPass();
            auto info = m_renderState->GetRenderPassInfo();
            // @TODO support multi level command buffers?
            //info.flags = m_level == VK_COMMAND_BUFFER_LEVEL_PRIMARY ? VK_RENDERING_CONTENTS_INLINE_BIT_KHR : VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;
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

        if ((flags & PK_RENDER_STATE_DIRTY_DESCRIPTOR_SETS) != 0)
        {
            auto bindBundle = m_renderState->GetDescriptorSetBundle(GetFenceRef(), flags);
            vkCmdBindDescriptorSets(m_commandBuffer, bindBundle.bindPoint, bindBundle.layout, bindBundle.firstSet, bindBundle.count, bindBundle.sets, 0, nullptr);
        }

        if (m_renderState->HasPipeline())
        {
            auto& constantLayout = m_renderState->GetPipelinePushConstantLayout();
            auto props = m_renderState->GetServices()->globalResources;

            for (auto& element : constantLayout)
            {
                const char* data = nullptr;
                size_t dataSize = 0u;

                if (props->TryGet<char>(element->name, data, &dataSize) && dataSize <= element->size)
                {
                    auto stageFlags = VulkanEnumConvert::GetShaderStageFlags(element->stageFlags);
                    vkCmdPushConstants(m_commandBuffer, m_renderState->GetPipelineLayout(), stageFlags, element->offset, (uint32_t)dataSize, data);
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

    void VulkanCommandBuffer::BeginRecord(VkCommandBuffer commandBuffer, VkFence fence, uint16_t queueFamily, VkCommandBufferLevel level, VulkanRenderState* renderState)
    {
        m_commandBuffer = commandBuffer;
        m_fence = fence;
        m_queueFamily = queueFamily;
        m_level = level;
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
        m_commandBuffer = VK_NULL_HANDLE; 
        m_fence = VK_NULL_HANDLE;
        ++m_invocationIndex;
    }
}
