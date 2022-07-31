#include "PrecompiledHeader.h"
#include "VulkanCommandBuffer.h"
#include "Utilities/Handle.h"
#include "Rendering/VulkanRHI/Objects/VulkanBuffer.h"
#include "Rendering/VulkanRHI/Objects/VulkanTexture.h"
#include "Rendering/VulkanRHI/VulkanWindow.h"
#include "Rendering/VulkanRHI/Objects/VulkanBindArray.h"
#include "Rendering/VulkanRHI/Utilities/VulkanExtensions.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Utilities;
    using namespace Utilities;
    using namespace Core;

    void VulkanCommandBuffer::SetRenderTarget(const uint3& resolution)
    {
        VulkanRenderTarget dummy(VK_NULL_HANDLE,
            VK_NULL_HANDLE,
            VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
            VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM,
            VK_FORMAT_UNDEFINED,
            { resolution.x, resolution.y, resolution.z },
            1u,
            1u);

        renderState->SetRenderTarget(&dummy, nullptr, 0u);
    }

    void VulkanCommandBuffer::SetRenderTarget(Texture** renderTargets, Texture** resolveTargets, const TextureViewRange* ranges, uint32_t count)
    {
        auto colors = PK_STACK_ALLOC(VulkanRenderTarget, count);
        auto resolves = resolveTargets != nullptr ? PK_STACK_ALLOC(VulkanRenderTarget, count) : nullptr;

        for (auto i = 0u; i < count; ++i)
        {
            auto color = renderTargets[i]->GetNative<VulkanTexture>()->GetRenderTarget(ranges[i]);
            memcpy(colors + i, &color, sizeof(VulkanRenderTarget));

            if (resolves != nullptr && resolveTargets[i] != nullptr)
            {
                auto resolve = resolveTargets[i]->GetNative<VulkanTexture>()->GetRenderTarget(ranges[i]);
                memcpy(resolves + i, &resolve, sizeof(VulkanRenderTarget));
            }
        }

        renderState->SetRenderTarget(colors, resolves, count);
    }

    void VulkanCommandBuffer::SetViewPorts(const uint4* rects, uint32_t count)
    {
        VkViewport* viewports = nullptr;
        if (renderState->SetViewports(rects, count, &viewports))
        {
            vkCmdSetViewport(commandBuffer, 0, count, viewports);
        }
    }

    void VulkanCommandBuffer::SetScissors(const uint4* rects, uint32_t count)
    {
        VkRect2D* scissors = nullptr;
        if (renderState->SetScissors(rects, count, &scissors))
        {
            vkCmdSetScissor(commandBuffer, 0, count, scissors);
        }
    }


    void VulkanCommandBuffer::SetShader(const Shader* shader, int32_t variantIndex)
    {
        if (variantIndex == -1)
        {
            auto selector = shader->GetVariantSelector();
            selector.SetKeywordsFrom(renderState->GetResourceState());
            variantIndex = selector.GetIndex();
        }

        auto pVariant = shader->GetVariant(variantIndex)->GetNative<VulkanShader>();
        auto& fixedAttrib = shader->GetFixedFunctionAttributes();

        // No need to assign raster params for a non graphics pipeline
        if (shader->GetType() == ShaderType::Graphics)
        {
            renderState->SetBlending(fixedAttrib.blending);
            renderState->SetDepthStencil(fixedAttrib.depthStencil);
            renderState->SetRasterization(fixedAttrib.rasterization);
        }

        renderState->SetShader(pVariant);
    }

    void VulkanCommandBuffer::SetVertexBuffers(const Buffer** buffers, uint32_t count)
    {
        auto pHandles = PK_STACK_ALLOC(const VulkanBindHandle*, count);

        for (auto i = 0u; i < count; ++i)
        {
            pHandles[i] = buffers[i]->GetNative<VulkanBuffer>()->GetBindHandle();
        }

        renderState->SetVertexBuffers(pHandles, count);
    }

    void VulkanCommandBuffer::SetIndexBuffer(const Buffer* buffer, size_t offset)
    {
        auto handle = buffer->GetNative<VulkanBuffer>()->GetBindHandle();
        renderState->SetIndexBuffer(handle, EnumConvert::GetIndexType(handle->bufferLayout->begin()->Type));
    }

    void VulkanCommandBuffer::SetBuffer(uint32_t nameHashId, Buffer* buffer, const IndexRange& range)
    {
        renderState->SetResource(nameHashId, Handle(buffer->GetNative<VulkanBuffer>()->GetBindHandle(range)));
    }

    void VulkanCommandBuffer::SetTexture(uint32_t nameHashId, Texture* texture, const TextureViewRange& range)
    {
        renderState->SetResource(nameHashId, Handle(texture->GetNative<VulkanTexture>()->GetBindHandle(range, true)));
    }

    void VulkanCommandBuffer::SetBufferArray(uint32_t nameHashId, BindArray<Buffer>* bufferArray)
    {
        renderState->SetResource(nameHashId, Handle(bufferArray->GetNative<VulkanBindArray>()));
    }

    void VulkanCommandBuffer::SetTextureArray(uint32_t nameHashId, BindArray<Texture>* textureArray)
    {
        renderState->SetResource(nameHashId, Handle(textureArray->GetNative<VulkanBindArray>()));
    }

    void VulkanCommandBuffer::SetImage(uint32_t nameHashId, Texture* texture, const TextureViewRange& range)
    {
        renderState->SetResource(nameHashId, Handle(texture->GetNative<VulkanTexture>()->GetBindHandle(range, false)));
    }

    void VulkanCommandBuffer::SetConstant(uint32_t nameHashId, const void* data, uint32_t size)
    {
        renderState->SetResource<char>(nameHashId, reinterpret_cast<const char*>(data), size);
    }

    void VulkanCommandBuffer::SetKeyword(uint32_t nameHashId, bool value)
    {
        renderState->SetResource<bool>(nameHashId, value);
    }


    void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        ValidatePipeline();
        vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        ValidatePipeline();
        vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndexedIndirect(const Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        ValidatePipeline();
        vkCmdDrawIndexedIndirect(commandBuffer, indirectArguments->GetNative<VulkanBuffer>()->GetRaw()->buffer, offset, drawCount, stride);
    }

    void VulkanCommandBuffer::Dispatch(uint3 groupCount)
    {
        EndRenderPass();
        ValidatePipeline();
        vkCmdDispatch(commandBuffer, groupCount.x, groupCount.y, groupCount.z);
    }


    void VulkanCommandBuffer::Blit(Texture* src, Window* dst,FilterMode filter)
    {
        auto vksrc = src->GetNative<VulkanTexture>();
        auto vkwindow = dst->GetNative<VulkanWindow>();
        Blit(vksrc->GetRenderTarget(), vkwindow->GetRenderTarget(), 0, 0, 0, 0, filter, true);
    }

    void VulkanCommandBuffer::Blit(Window* src, Buffer* dst)
    {
        auto vksrc = src->GetNative<VulkanWindow>()->GetRenderTarget();
        auto vkbuff = dst->GetNative<VulkanBuffer>();

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = vksrc.extent;

        VkImageSubresourceRange srcRange{};
        srcRange.aspectMask = vksrc.aspect;
        srcRange.baseMipLevel = 0;
        srcRange.levelCount = 1;
        srcRange.baseArrayLayer = 0;
        srcRange.layerCount = 1;

        TransitionImageLayout(VulkanLayoutTransition(vksrc.image, vksrc.layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcRange));
        vkCmdCopyImageToBuffer(commandBuffer, vksrc.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkbuff->GetRaw()->buffer, 1, &region);
        TransitionImageLayout(VulkanLayoutTransition(vksrc.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vksrc.layout, srcRange));
    }

    void VulkanCommandBuffer::Blit(Texture* src, Texture* dst, uint32_t srcLevel, uint32_t dstLevel, uint32_t srcLayer, uint32_t dstLayer, FilterMode filter)
    {
        auto vksrc = src->GetNative<VulkanTexture>();
        auto vkdst = dst->GetNative<VulkanTexture>();
        Blit(vksrc->GetRenderTarget(), vkdst->GetRenderTarget(), 0, dstLevel, 0, dstLayer, filter);
    }

    void VulkanCommandBuffer::Blit(const VulkanRenderTarget& src, 
                                   const VulkanRenderTarget& dst, 
                                   uint32_t srcLevel, 
                                   uint32_t dstLevel, 
                                   uint32_t srcLayer, 
                                   uint32_t dstLayer,
                                   FilterMode filter,
                                   bool flipVertical)
    {
        VkImageBlit blitRegion{};
        blitRegion.srcSubresource = { (uint32_t)src.aspect, srcLevel, srcLayer, 1 };
        blitRegion.dstSubresource = { (uint32_t)dst.aspect, dstLevel, dstLayer, 1 };
        blitRegion.srcOffsets[1] = { (int)src.extent.width, (int)src.extent.height, (int)src.extent.depth };
        blitRegion.dstOffsets[1] = { (int)dst.extent.width, (int)dst.extent.height, (int)dst.extent.depth };

        if (flipVertical)
        {
            blitRegion.srcOffsets[0].y = (int)src.extent.height;
            blitRegion.srcOffsets[1].y = 0u;
        }

        VkImageResolve resolveRegion{};
        resolveRegion.srcSubresource = { (uint32_t)src.aspect, srcLevel, srcLayer, 1 };
        resolveRegion.dstSubresource = { (uint32_t)dst.aspect, dstLevel, dstLayer, 1 };
        resolveRegion.extent = src.extent;

        VkImageSubresourceRange srcRange{};
        srcRange.aspectMask = src.aspect;
        srcRange.baseMipLevel = srcLevel;
        srcRange.levelCount = 1;
        srcRange.baseArrayLayer = srcLayer;
        srcRange.layerCount = 1;

        VkImageSubresourceRange dstRange{};
        dstRange.aspectMask = dst.aspect;
        dstRange.baseMipLevel = dstLevel;
        dstRange.levelCount = 1;
        dstRange.baseArrayLayer = dstLayer;
        dstRange.layerCount = 1;

        TransitionImageLayout(VulkanLayoutTransition(src.image, src.layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcRange));
        TransitionImageLayout(VulkanLayoutTransition(dst.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstRange));

        if (src.samples > 1 && dst.samples == 1)
        {
            vkCmdResolveImage(commandBuffer, src.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &resolveRegion);
        }
        else 
        {
            auto vkFilter = EnumConvert::GetFilterMode(filter);
            vkCmdBlitImage(commandBuffer, src.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, vkFilter);
        }

        TransitionImageLayout(VulkanLayoutTransition(src.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src.layout, srcRange));
        TransitionImageLayout(VulkanLayoutTransition(dst.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst.layout, dstRange));
    }

    void VulkanCommandBuffer::Clear(Buffer* dst, size_t offset, size_t size, uint32_t value)
    {
        EndRenderPass();
        vkCmdFillBuffer(commandBuffer, dst->GetNative<VulkanBuffer>()->GetRaw()->buffer, offset, size, value);
    }

    void VulkanCommandBuffer::Clear(Texture* dst, const TextureViewRange& range, const uint4& value)
    {
        auto vktex = dst->GetNative<VulkanTexture>();
        auto rawtex = vktex->GetRaw();
        auto handle = vktex->GetBindHandle(range, false);
        auto normalizedRange = vktex->NormalizeViewRange(range);
        
        VkImageSubresourceRange subrange{};
        subrange.aspectMask = rawtex->aspect;
        subrange.baseMipLevel = normalizedRange.level;
        subrange.levelCount = normalizedRange.levels;
        subrange.baseArrayLayer = normalizedRange.layer;
        subrange.layerCount = normalizedRange.layers;

        VkClearColorValue clearValue{};

        for (auto i = 0u; i < 4; ++i)
        {
            clearValue.float32[i] = *reinterpret_cast<const float*>(&value[i]);
            clearValue.int32[i] = *reinterpret_cast<const int32_t*>(&value[i]);
            clearValue.uint32[i] = value[i];
        }

        vkCmdClearColorImage(commandBuffer, vktex->GetRaw()->image, handle->imageLayout, &clearValue, 1, &subrange);
    }


    void VulkanCommandBuffer::Barrier(const Texture* texture, const TextureViewRange& range, const Buffer* buffer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags)
    {
        VkImageMemoryBarrier imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        VkMemoryBarrier memoryBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        VkBufferMemoryBarrier bufferBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};

        if (texture != nullptr)
        {
            auto vktex = texture->GetNative<VulkanTexture>();
            auto vkrawtex = vktex->GetRaw();
            auto normalizedRange = vktex->NormalizeViewRange(range);
            imageBarrier.oldLayout = imageBarrier.newLayout = vktex->GetImageLayout();
            imageBarrier.image = vkrawtex->image;
            imageBarrier.subresourceRange =
            {
                    (uint32_t)vkrawtex->aspect,
                    normalizedRange.level,
                    normalizedRange.levels,
                    normalizedRange.layer,
                    normalizedRange.layers
            };
        }

        if (buffer != nullptr)
        {
            auto vkbuff = buffer->GetNative<VulkanBuffer>()->GetRaw();
            bufferBarrier.buffer = vkbuff->buffer;
            bufferBarrier.size = vkbuff->capacity;
        }

        if (texture == nullptr && buffer == nullptr)
        {
            memoryBarrier.srcAccessMask;
            memoryBarrier.dstAccessMask;
        }

        imageBarrier.srcQueueFamilyIndex = bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.srcAccessMask = imageBarrier.srcAccessMask = bufferBarrier.srcAccessMask = EnumConvert::GetAccessFlags(srcFlags);
        memoryBarrier.dstAccessMask = imageBarrier.dstAccessMask = bufferBarrier.dstAccessMask = EnumConvert::GetAccessFlags(dstFlags);

        PipelineBarrier
        (
            EnumConvert::GetPipelineStageFlags(srcFlags),
            EnumConvert::GetPipelineStageFlags(dstFlags),
            0,
            texture == nullptr && buffer == nullptr ? 1u : 0u,
            &memoryBarrier,
            buffer != nullptr ? 1u : 0u,
            &bufferBarrier,
            texture != nullptr ? 1u : 0u,
            &imageBarrier
        );
    }

    void VulkanCommandBuffer::BeginDebugScope(const char* name, const Math::color& color)
    {
        VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
        labelInfo.pNext = nullptr;
        labelInfo.pLabelName = name;
        memcpy(labelInfo.color, glm::value_ptr(color), sizeof(Math::color));
        vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &labelInfo);
    }

    void VulkanCommandBuffer::EndDebugScope()
    {
        vkCmdEndDebugUtilsLabelEXT(commandBuffer);
    }

    void VulkanCommandBuffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const
    {
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
    }

    void VulkanCommandBuffer::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) const
    {
        vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
    }

    void VulkanCommandBuffer::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, const VkExtent3D& extent, uint32_t level, uint32_t layer) const
    {
        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = level;
        region.imageSubresource.baseArrayLayer = layer;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = extent;
        CopyBufferToImage(srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    void VulkanCommandBuffer::TransitionImageLayout(const VulkanLayoutTransition& transition)
    {
        if (transition.oldLayout == transition.newLayout)
        {
            return;
        }

        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = transition.oldLayout;
        barrier.newLayout = transition.newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = transition.image;
        barrier.subresourceRange = transition.subresources;
        barrier.srcAccessMask = transition.srcAccessMask;
        barrier.dstAccessMask = transition.dstAccessMask;
        EndRenderPass();
        PipelineBarrier(transition.srcStage, transition.dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void VulkanCommandBuffer::PipelineBarrier(VkPipelineStageFlags srcStageMask, 
                                              VkPipelineStageFlags dstStageMask, 
                                              VkDependencyFlags dependencyFlags, 
                                              uint32_t memoryBarrierCount, 
                                              const VkMemoryBarrier* pMemoryBarriers,
                                              uint32_t bufferMemoryBarrierCount,
                                              const VkBufferMemoryBarrier* pBufferMemoryBarriers, 
                                              uint32_t imageMemoryBarrierCount, 
                                              const VkImageMemoryBarrier* pImageMemoryBarriers)
    {

        // Memory & buffer memory barriers not allowed inside renderpasses. Barriers are not allowed inside renderpasses unless using self-dependencies.
        if (memoryBarrierCount > 0 || bufferMemoryBarrierCount > 0 || !renderState->HasDynamicTargets())
        {
            EndRenderPass();
        }

        vkCmdPipelineBarrier(commandBuffer,
            srcStageMask,
            dstStageMask,
            dependencyFlags,
            memoryBarrierCount,
            pMemoryBarriers,
            bufferMemoryBarrierCount,
            pBufferMemoryBarriers,
            imageMemoryBarrierCount,
            pImageMemoryBarriers);
    }  


    void VulkanCommandBuffer::ValidatePipeline()
    {
        auto flags = renderState->ValidatePipeline(GetOnCompleteGate());

        if ((flags & PK_RENDER_STATE_DIRTY_RENDERTARGET) != 0)
        {
            EndRenderPass();
            auto info = renderState->GetRenderPassInfo();
            vkCmdBeginRenderPass(commandBuffer, &info, level == VK_COMMAND_BUFFER_LEVEL_PRIMARY ? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
            isInActiveRenderPass = true;
        }

        if ((flags & PK_RENDER_STATE_DIRTY_PIPELINE) != 0)
        {
            vkCmdBindPipeline(commandBuffer, renderState->GetPipelineBindPoint(), renderState->GetPipeline());
        }

        if ((flags & PK_RENDER_STATE_DIRTY_VERTEXBUFFERS) != 0)
        {
            auto vertexBufferBundle = renderState->GetVertexBufferBundle();

            if (vertexBufferBundle.count > 0)
            {
                vkCmdBindVertexBuffers(commandBuffer, 0, vertexBufferBundle.count, vertexBufferBundle.buffers, vertexBufferBundle.offsets);
            }
        }

        if ((flags & PK_RENDER_STATE_DIRTY_INDEXBUFFER) != 0)
        {
            VkIndexType indexType; 
            auto indexBufferHandle = renderState->GetIndexBuffer(&indexType);
            vkCmdBindIndexBuffer(commandBuffer, indexBufferHandle->buffer, indexBufferHandle->bufferOffset, indexType);
        }

        if ((flags & PK_RENDER_STATE_DIRTY_DESCRIPTOR_SETS) != 0)
        {
            auto bindBundle = renderState->GetDescriptorSetBundle(GetOnCompleteGate(), flags);
            vkCmdBindDescriptorSets(commandBuffer, bindBundle.bindPoint, bindBundle.layout, bindBundle.firstSet, bindBundle.count, bindBundle.sets, 0, nullptr);
        }

        if (renderState->HasPipeline())
        {
            auto& constantLayout = renderState->GetPipelinePushConstantLayout();
            auto& props = renderState->GetResourceState();

            for (auto& kv : constantLayout)
            {
                const char* data = nullptr;
                size_t dataSize = 0u;
                auto& element = kv.second;

                if (props.TryGet<char>(kv.second.NameHashId, data, &dataSize) && dataSize <= element.Size)
                {
                    vkCmdPushConstants(commandBuffer, renderState->GetPipelineLayout(), element.StageFlags, element.Offset, (uint32_t)dataSize, data);
                }
            }
        }
    }

    void VulkanCommandBuffer::EndRenderPass()
    {
        if (isInActiveRenderPass)
        {
            vkCmdEndRenderPass(commandBuffer);
            isInActiveRenderPass = false;
        }
    }
}
