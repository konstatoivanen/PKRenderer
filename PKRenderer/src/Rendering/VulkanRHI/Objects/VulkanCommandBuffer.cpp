#include "PrecompiledHeader.h"
#include "VulkanCommandBuffer.h"
#include "Utilities/Handle.h"
#include "Rendering/VulkanRHI/Objects/VulkanBuffer.h"
#include "Rendering/VulkanRHI/Objects/VulkanTexture.h"
#include "Rendering/VulkanRHI/Objects/VulkanAccelerationStructure.h"
#include "Rendering/VulkanRHI/VulkanWindow.h"
#include "Rendering/VulkanRHI/Objects/VulkanBindArray.h"
#include "Rendering/VulkanRHI/Utilities/VulkanExtensions.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Rendering::VulkanRHI::Services;
    using namespace PK::Utilities;
    using namespace Utilities;
    using namespace Core;

    FenceRef VulkanCommandBuffer::GetFenceRef() const
    {
        return FenceRef(this, [](const void* ctx, uint64_t userdata, uint64_t timeout)
            {
                auto cmd = reinterpret_cast<const VulkanCommandBuffer*>(ctx);
                return cmd->m_invocationIndex >= userdata;
            },
            m_invocationIndex + 1);
    }

    void VulkanCommandBuffer::SetRenderTarget(const uint3& resolution)
    {
        // @TODO move somewhere non static
        static VulkanBindHandle dummy{};
        dummy.image.image = VK_NULL_HANDLE;
        dummy.image.view = VK_NULL_HANDLE;
        dummy.image.sampler = VK_NULL_HANDLE;
        dummy.image.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        dummy.image.format = VK_FORMAT_UNDEFINED;
        dummy.image.extent = { resolution.x, resolution.y, resolution.z };
        dummy.image.range = { VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM, 0u, 1u, 0u, 1u };
        dummy.image.samples = 1u;
        const VulkanBindHandle* handles = &dummy;
        m_renderState->SetRenderTarget(&handles, nullptr, 0u);
    }

    void VulkanCommandBuffer::SetRenderTarget(Texture** renderTargets, Texture** resolveTargets, const TextureViewRange* ranges, uint32_t count)
    {
        auto colors = PK_STACK_ALLOC(const VulkanBindHandle*, count);
        auto resolves = resolveTargets != nullptr ? PK_STACK_ALLOC(const VulkanBindHandle*, count) : nullptr;

        for (auto i = 0u; i < count; ++i)
        {
            colors[i] = renderTargets[i]->GetNative<VulkanTexture>()->GetBindHandle(ranges[i], TextureBindMode::RenderTarget);

            if (resolves != nullptr && resolveTargets[i] != nullptr)
            {
                resolves[i] = resolveTargets[i]->GetNative<VulkanTexture>()->GetBindHandle(ranges[i], TextureBindMode::RenderTarget);
            }
        }

        m_renderState->SetRenderTarget(colors, resolves, count);
    }

    void VulkanCommandBuffer::SetViewPorts(const uint4* rects, uint32_t count)
    {
        VkViewport* viewports = nullptr;
        if (m_renderState->SetViewports(rects, count, &viewports))
        {
            vkCmdSetViewport(m_commandBuffer, 0, count, viewports);
        }
    }

    void VulkanCommandBuffer::SetScissors(const uint4* rects, uint32_t count)
    {
        VkRect2D* scissors = nullptr;
        if (m_renderState->SetScissors(rects, count, &scissors))
        {
            vkCmdSetScissor(m_commandBuffer, 0, count, scissors);
        }
    }


    void VulkanCommandBuffer::SetShader(const Shader* shader, int32_t variantIndex)
    {
        if (variantIndex == -1)
        {
            auto selector = shader->GetVariantSelector();
            selector.SetKeywordsFrom(*m_renderState->GetServices()->globalResources);
            variantIndex = selector.GetIndex();
        }

        auto pVariant = shader->GetVariant(variantIndex)->GetNative<VulkanShader>();
        auto& fixedAttrib = shader->GetFixedFunctionAttributes();

        // No need to assign raster params for a non graphics pipeline
        if (shader->GetType() == ShaderType::Graphics)
        {
            m_renderState->SetBlending(fixedAttrib.blending);
            m_renderState->SetDepthStencil(fixedAttrib.depthStencil);
            m_renderState->SetRasterization(fixedAttrib.rasterization);
        }

        m_renderState->SetShader(pVariant);
    }

    void VulkanCommandBuffer::SetVertexBuffers(const Buffer** buffers, uint32_t count)
    {
        auto pHandles = PK_STACK_ALLOC(const VulkanBindHandle*, count);

        for (auto i = 0u; i < count; ++i)
        {
            pHandles[i] = buffers[i]->GetNative<VulkanBuffer>()->GetBindHandle();
        }

        m_renderState->SetVertexBuffers(pHandles, count);
    }

    void VulkanCommandBuffer::SetIndexBuffer(const Buffer* buffer, size_t offset)
    {
        auto handle = buffer->GetNative<VulkanBuffer>()->GetBindHandle();
        m_renderState->SetIndexBuffer(handle, EnumConvert::GetIndexType(handle->buffer.layout->begin()->Type));
    }

    void VulkanCommandBuffer::SetBuffer(uint32_t nameHashId, Buffer* buffer, const IndexRange& range)
    {
        auto resources = m_renderState->GetServices()->globalResources;
        resources->Set(nameHashId, Handle(buffer->GetNative<VulkanBuffer>()->GetBindHandle(range)));
    }

    void VulkanCommandBuffer::SetTexture(uint32_t nameHashId, Texture* texture, const TextureViewRange& range)
    {
        auto resources = m_renderState->GetServices()->globalResources;
        resources->Set(nameHashId, Handle(texture->GetNative<VulkanTexture>()->GetBindHandle(range, TextureBindMode::SampledTexture)));
    }

    void VulkanCommandBuffer::SetBufferArray(uint32_t nameHashId, BindArray<Buffer>* bufferArray)
    {
        auto resources = m_renderState->GetServices()->globalResources;
        resources->Set(nameHashId, Handle(bufferArray->GetNative<VulkanBindArray>()));
    }

    void VulkanCommandBuffer::SetTextureArray(uint32_t nameHashId, BindArray<Texture>* textureArray)
    {
        auto resources = m_renderState->GetServices()->globalResources;
        resources->Set(nameHashId, Handle(textureArray->GetNative<VulkanBindArray>()));
    }

    void VulkanCommandBuffer::SetImage(uint32_t nameHashId, Texture* texture, const TextureViewRange& range)
    {
        auto resources = m_renderState->GetServices()->globalResources;
        resources->Set(nameHashId, Handle(texture->GetNative<VulkanTexture>()->GetBindHandle(range, TextureBindMode::Image)));
    }

    void VulkanCommandBuffer::SetAccelerationStructure(uint32_t nameHashId, AccelerationStructure* structure)
    {
        auto resources = m_renderState->GetServices()->globalResources;
        resources->Set(nameHashId, Handle(structure->GetNative<VulkanAccelerationStructure>()->GetBindHandle()));
    }

    void VulkanCommandBuffer::SetShaderBindingTable(Structs::RayTracingShaderGroup group, const Buffer* buffer, size_t offset, size_t stride, size_t size)
    {
        auto address = buffer->GetNative<VulkanBuffer>()->GetRaw()->deviceAddress;
        m_renderState->SetShaderBindingTableAddress(group, address + offset, stride, size);
    }

    void VulkanCommandBuffer::SetConstant(uint32_t nameHashId, const void* data, uint32_t size)
    {
        auto resources = m_renderState->GetServices()->globalResources;
        resources->Set<char>(nameHashId, reinterpret_cast<const char*>(data), size);
    }

    void VulkanCommandBuffer::SetKeyword(uint32_t nameHashId, bool value)
    {
        auto resources = m_renderState->GetServices()->globalResources;
        resources->Set<bool>(nameHashId, value);
    }

    void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        ValidatePipeline();
        vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        ValidatePipeline();
        vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndexedIndirect(const Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        auto vkbuffer = indirectArguments->GetNative<VulkanBuffer>()->GetRaw();
        static Services::VulkanBarrierHandler::AccessRecord record{};
        record.bufferRange.offset = (uint32_t)offset;
        record.bufferRange.size = drawCount * stride;
        record.stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        record.access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        record.queueFamily = indirectArguments->IsConcurrent() ? VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(vkbuffer->buffer, record, PK_ACCESS_OPT_BARRIER);

        ValidatePipeline();
        vkCmdDrawIndexedIndirect(m_commandBuffer, vkbuffer->buffer, offset, drawCount, stride);
    }

    void VulkanCommandBuffer::Dispatch(uint3 groupCount)
    {
        EndRenderPass();
        ValidatePipeline();
        vkCmdDispatch(m_commandBuffer, groupCount.x, groupCount.y, groupCount.z);
    }

    void VulkanCommandBuffer::DispatchRays(Math::uint3 dimensions)
    {
        EndRenderPass();
        ValidatePipeline();
        auto addresses = m_renderState->GetShaderBindingTableAddresses();
        vkCmdTraceRaysKHR(m_commandBuffer,
                          addresses + (uint32_t)Structs::RayTracingShaderGroup::RayGeneration, 
                          addresses + (uint32_t)Structs::RayTracingShaderGroup::Miss,
                          addresses + (uint32_t)Structs::RayTracingShaderGroup::Hit,
                          addresses + (uint32_t)Structs::RayTracingShaderGroup::Callable,
                          dimensions.x, 
                          dimensions.y, 
                          dimensions.z);
    }


    void VulkanCommandBuffer::Blit(Texture* src, Window* dst, FilterMode filter)
    {
        auto vksrc = src->GetNative<VulkanTexture>();
        auto vkwindow = dst->GetNative<VulkanWindow>();
        const auto& srcHandle = vksrc->GetBindHandle(TextureBindMode::RenderTarget);
        const auto& windowHandle = vkwindow->GetBindHandle();
        
        Blit(srcHandle, windowHandle, 0, 0, 0, 0, filter, true);

        m_renderState->RecordImage(windowHandle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
        
        // Resolve swapchain image layout immediately
        ResolveBarriers();
    }

    void VulkanCommandBuffer::Blit(Window* src, Buffer* dst)
    {
        auto vksrc = src->GetNative<VulkanWindow>()->GetBindHandle();
        auto vkbuff = dst->GetNative<VulkanBuffer>();

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = vksrc->image.range.aspectMask;
        region.imageSubresource.mipLevel = vksrc->image.range.baseMipLevel;
        region.imageSubresource.baseArrayLayer = vksrc->image.range.baseArrayLayer;
        region.imageSubresource.layerCount = vksrc->image.range.layerCount;
        region.imageExtent = vksrc->image.extent;

        m_renderState->RecordImage(vksrc, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        EndRenderPass();
        ResolveBarriers();
        vkCmdCopyImageToBuffer(m_commandBuffer, vksrc->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkbuff->GetRaw()->buffer, 1, &region);
    }

    void VulkanCommandBuffer::Blit(Texture* src, Texture* dst, const Structs::TextureViewRange& srcRange, const Structs::TextureViewRange& dstRange, FilterMode filter)
    {
        static VulkanBindHandle srcHandle;
        static VulkanBindHandle dstHandle;
        src->GetNative<VulkanTexture>()->FillBindHandle(&srcHandle, srcRange, TextureBindMode::RenderTarget);
        dst->GetNative<VulkanTexture>()->FillBindHandle(&dstHandle, dstRange, TextureBindMode::RenderTarget);
        Blit(&srcHandle, &dstHandle, srcRange.level, dstRange.level, srcRange.layer, dstRange.layer, filter);
    }

    void VulkanCommandBuffer::Blit(const VulkanBindHandle* src, 
                                   const VulkanBindHandle* dst, 
                                   uint32_t srcLevel, 
                                   uint32_t dstLevel, 
                                   uint32_t srcLayer, 
                                   uint32_t dstLayer,
                                   FilterMode filter,
                                   bool flipVertical)
    {
        VkImageBlit blitRegion{};
        blitRegion.srcSubresource = { (uint32_t)src->image.range.aspectMask, srcLevel, srcLayer, src->image.range.layerCount };
        blitRegion.dstSubresource = { (uint32_t)dst->image.range.aspectMask, dstLevel, dstLayer, dst->image.range.layerCount };
        blitRegion.srcOffsets[1] = { (int)src->image.extent.width, (int)src->image.extent.height, (int)src->image.extent.depth };
        blitRegion.dstOffsets[1] = { (int)dst->image.extent.width, (int)dst->image.extent.height, (int)dst->image.extent.depth };

        if (flipVertical)
        {
            blitRegion.srcOffsets[0].y = (int)src->image.extent.height;
            blitRegion.srcOffsets[1].y = 0u;
        }

        VkImageResolve resolveRegion{};
        resolveRegion.srcSubresource = { (uint32_t)src->image.range.aspectMask, srcLevel, srcLayer, src->image.range.layerCount };
        resolveRegion.dstSubresource = { (uint32_t)dst->image.range.aspectMask, dstLevel, dstLayer, dst->image.range.layerCount };
        resolveRegion.extent = src->image.extent;

        m_renderState->RecordImage(src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        m_renderState->RecordImage(dst, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0u, VK_IMAGE_LAYOUT_UNDEFINED, 0u);
        m_renderState->RecordImage(dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        EndRenderPass();
        ResolveBarriers();

        if (src->image.samples > 1 && dst->image.samples == 1)
        {
            vkCmdResolveImage(m_commandBuffer, src->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &resolveRegion);
        }
        else 
        {
            auto vkFilter = EnumConvert::GetFilterMode(filter);
            vkCmdBlitImage(m_commandBuffer, src->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, vkFilter);
        }
    }

    void VulkanCommandBuffer::Clear(Buffer* dst, size_t offset, size_t size, uint32_t value)
    {
        EndRenderPass();
        vkCmdFillBuffer(m_commandBuffer, dst->GetNative<VulkanBuffer>()->GetRaw()->buffer, offset, size, value);
    }

    void VulkanCommandBuffer::Clear(Texture* dst, const TextureViewRange& range, const uint4& value)
    {
        auto vktex = dst->GetNative<VulkanTexture>();
        auto rawtex = vktex->GetRaw();
        auto handle = vktex->GetBindHandle(range, TextureBindMode::Image);
        auto normalizedRange = vktex->NormalizeViewRange(range);
        
        VkImageSubresourceRange subrange{};
        subrange.aspectMask = rawtex->aspect;
        subrange.baseMipLevel = normalizedRange.level;
        subrange.levelCount = normalizedRange.levels;
        subrange.baseArrayLayer = normalizedRange.layer;
        subrange.layerCount = normalizedRange.layers;

        VkClearColorValue clearValue{};
        memcpy(clearValue.uint32, glm::value_ptr(value), sizeof(clearValue.uint32));
        vkCmdClearColorImage(m_commandBuffer, vktex->GetRaw()->image, handle->image.layout, &clearValue, 1, &subrange);
    }

    void* VulkanCommandBuffer::BeginBufferWrite(Buffer* buffer, size_t offset, size_t size)
    {
        return buffer->GetNative<VulkanBuffer>()->BeginWrite(GetFenceRef(), offset, size);
    }

    void VulkanCommandBuffer::EndBufferWrite(Buffer* buffer)
    {
        VkBufferCopy copyRegion;
        VkBuffer srcBuffer, dstBuffer;

        auto vkBuffer = buffer->GetNative<VulkanBuffer>();
        vkBuffer->EndWrite(&srcBuffer, &dstBuffer, &copyRegion);

        vkCmdCopyBuffer(m_commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        static Services::VulkanBarrierHandler::AccessRecord record{};
        record.bufferRange.offset = (uint32_t)copyRegion.dstOffset;
        record.bufferRange.size = (uint32_t)copyRegion.size;
        record.stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        record.access = VK_ACCESS_TRANSFER_WRITE_BIT;
        record.queueFamily = buffer->IsConcurrent() ? VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
        m_renderState->GetServices()->barrierHandler->Record(dstBuffer, record, PK_ACCESS_OPT_BARRIER);
    }

    void VulkanCommandBuffer::UploadTexture(Texture* texture, const void* data, size_t size, Structs::ImageUploadRange* ranges, uint32_t rangeCount)
    {
        auto vkTexture = texture->GetNative<VulkanTexture>();
		auto layout = vkTexture->GetImageLayout();
		auto range = VkImageSubresourceRange { (uint32_t)vkTexture->GetAspectFlags(), 0, texture->GetLevels(), 0, texture->GetLayers() };
        const auto stage = m_renderState->GetServices()->stagingBufferCache->GetBuffer(size, GetFenceRef());
		std::vector<VkBufferImageCopy> bufferCopyRegions;
        bufferCopyRegions.reserve(rangeCount);

        for (auto i = 0u; i < rangeCount; ++i)
        {
            auto& range = ranges[i];
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = vkTexture->GetAspectFlags();
            bufferCopyRegion.imageSubresource.mipLevel = range.level;
            bufferCopyRegion.imageSubresource.mipLevel= range.level;
            bufferCopyRegion.imageSubresource.baseArrayLayer = range.layer;
			bufferCopyRegion.imageSubresource.layerCount = range.layers;
            bufferCopyRegion.imageExtent.width = range.extent.x;
            bufferCopyRegion.imageExtent.height = range.extent.y;
            bufferCopyRegion.imageExtent.depth = range.extent.z;
            bufferCopyRegion.imageOffset.x = range.offset.x;
            bufferCopyRegion.imageOffset.y = range.offset.y;
            bufferCopyRegion.imageOffset.z = range.offset.z;
			bufferCopyRegion.bufferOffset = range.bufferOffset;
			bufferCopyRegions.push_back(bufferCopyRegion);
		}

		stage->SetData(data, size);
		TransitionImageLayout(vkTexture->GetRaw()->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
        vkCmdCopyBufferToImage(m_commandBuffer, stage->buffer, vkTexture->GetRaw()->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)bufferCopyRegions.size(), bufferCopyRegions.data());
		TransitionImageLayout(vkTexture->GetRaw()->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, range);
    }

    void VulkanCommandBuffer::UploadTexture(Texture* texture, const void* data, size_t size, uint32_t level, uint32_t layer)
    {
        auto vkTexture = texture->GetNative<VulkanTexture>();
        auto extent = vkTexture->GetRaw()->extent;
        auto image = vkTexture->GetRaw()->image;
        auto layout = vkTexture->GetImageLayout();
        auto range = VkImageSubresourceRange { (uint32_t)vkTexture->GetAspectFlags(), level, 1, layer, 1 };
        const auto* stage = m_renderState->GetServices()->stagingBufferCache->GetBuffer(size, GetFenceRef());
        
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
        vkCmdCopyBufferToImage(m_commandBuffer, stage->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copyRegion);
        TransitionImageLayout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, range);
    }

    void VulkanCommandBuffer::BeginDebugScope(const char* name, const Math::color& color)
    {
        VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
        labelInfo.pNext = nullptr;
        labelInfo.pLabelName = name;
        memcpy(labelInfo.color, glm::value_ptr(color), sizeof(Math::color));
        vkCmdBeginDebugUtilsLabelEXT(m_commandBuffer, &labelInfo);
    }

    void VulkanCommandBuffer::EndDebugScope()
    {
        vkCmdEndDebugUtilsLabelEXT(m_commandBuffer);
    }

    void VulkanCommandBuffer::BuildAccelerationStructures(uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
    {
        vkCmdBuildAccelerationStructuresKHR(m_commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
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
        // Memory & buffer memory barriers not allowed inside renderpasses. Barriers are not allowed inside renderpasses unless using self-dependencies.
        if (barrier.memoryBarrierCount > 0 || barrier.bufferMemoryBarrierCount > 0 || !m_renderState->HasDynamicTargets())
        {
            EndRenderPass();
        }

        vkCmdPipelineBarrier(m_commandBuffer,
            barrier.srcStageMask,
            barrier.dstStageMask,
            barrier.dependencyFlags,
            barrier.memoryBarrierCount,
            barrier.pMemoryBarriers,
            barrier.bufferMemoryBarrierCount,
            barrier.pBufferMemoryBarriers,
            barrier.imageMemoryBarrierCount,
            barrier.pImageMemoryBarriers);
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
            vkCmdBeginRenderPass(m_commandBuffer, &info, m_level == VK_COMMAND_BUFFER_LEVEL_PRIMARY ? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
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

            for (auto& kv : constantLayout)
            {
                const char* data = nullptr;
                size_t dataSize = 0u;
                auto& element = kv.second;

                if (props->TryGet<char>(kv.second.NameHashId, data, &dataSize) && dataSize <= element.Size)
                {
                    auto stageFlags = EnumConvert::GetShaderStageFlags(element.StageFlags);
                    vkCmdPushConstants(m_commandBuffer, m_renderState->GetPipelineLayout(), stageFlags, element.Offset, (uint32_t)dataSize, data);
                }
            }
        }
    }

    void VulkanCommandBuffer::EndRenderPass()
    {
        if (m_isInActiveRenderPass)
        {
            vkCmdEndRenderPass(m_commandBuffer);
            m_isInActiveRenderPass = false;
        }
    }
    
    void VulkanCommandBuffer::BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferLevel level, Objects::VulkanRenderState* renderState)
    {
        m_level = level;
        m_commandBuffer = commandBuffer;
        m_renderState = renderState;
        m_renderState->Reset();

        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_ASSERT_RESULT(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
    }

    void VulkanCommandBuffer::EndCommandBuffer(VulkanBarrierInfo* transferBarrier)
    {
        // End possibly active render pass
        EndRenderPass();

        //Add potential resource transitions
        VulkanBarrierInfo barrier;
        if (m_renderState->GetServices()->barrierHandler->Resolve(&barrier))
        {
            PipelineBarrier(barrier);
            
            if (transferBarrier)
            {
                *transferBarrier = barrier;
            }
        }

        VK_ASSERT_RESULT(vkEndCommandBuffer(m_commandBuffer));
        m_renderState = nullptr;
    }
}
