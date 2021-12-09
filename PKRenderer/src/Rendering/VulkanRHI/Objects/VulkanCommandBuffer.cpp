#include "PrecompiledHeader.h"
#include "VulkanCommandBuffer.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/VulkanRHI/Objects/VulkanBuffer.h"
#include "Rendering/VulkanRHI/Objects/VulkanTexture.h"
#include "Rendering/VulkanRHI/VulkanWindow.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    void VulkanCommandBuffer::SetRenderTarget(Window* window, uint32_t index)
    {
        renderState.SetRenderTarget(window->GetNative<VulkanWindow>()->GetRenderTarget(), index);
    }

    void VulkanCommandBuffer::SetRenderTarget(Texture* renderTarget, uint32_t index)
    {
        renderState.SetRenderTarget(renderTarget->GetNative<VulkanTexture>()->GetRenderTarget(), index);
    }

    void VulkanCommandBuffer::SetViewPort(uint4 rect, float mindepth, float maxdepth)
    {
        SetViewPort({ { (int)rect.x, (int)rect.y }, { rect.z, rect.w } }, mindepth, maxdepth);
    }

    void VulkanCommandBuffer::SetScissor(uint4 rect)
    {
        SetScissor({ { (int)rect.x, (int)rect.y }, { rect.z, rect.w } });
    }

    void VulkanCommandBuffer::SetShader(const Shader* shader, uint variantIndex)
    {
        auto pVariant = shader->GetVariant(variantIndex)->GetNative<VulkanShader>();
        auto& fixedAttrib = shader->GetFixedFunctionAttributes();
        renderState.SetBlending(fixedAttrib.blending);
        renderState.SetDepthStencil(fixedAttrib.depthStencil);
        renderState.SetRasterization(fixedAttrib.rasterization);
        renderState.SetShader(pVariant);
    }

    void VulkanCommandBuffer::SetVertexBuffers(const Buffer** buffers, uint count)
    {
        auto pHandles = PK_STACK_ALLOC(const VulkanBindHandle*, count);

        for (auto i = 0u; i < count; ++i)
        {
            pHandles[i] = buffers[i]->GetNative<VulkanBuffer>()->GetBindHandle();
        }

        renderState.SetVertexBuffers(pHandles, count);
    }

    void VulkanCommandBuffer::SetIndexBuffer(const Buffer* buffer, size_t offset)
    {
        auto handle = buffer->GetNative<VulkanBuffer>()->GetBindHandle();
        BindIndexBuffer(handle->buffer, offset, EnumConvert::GetIndexType(handle->bufferLayout->begin()->Type));
    }

    void VulkanCommandBuffer::SetBuffer(uint32_t nameHashId, const Buffer* buffer)
    {
        renderState.SetResource(nameHashId, buffer->GetNative<VulkanBuffer>()->GetBindHandle());
    }

    void VulkanCommandBuffer::SetTexture(uint32_t nameHashId, Texture* texture)
    {
        renderState.SetResource(nameHashId, texture->GetNative<VulkanTexture>()->GetBindHandle());
    }

    void VulkanCommandBuffer::SetConstant(uint32_t nameHashId, const void* data, uint32_t size)
    {
        if (renderState.pipelineKey.shader == nullptr)
        {
            return;
        }

        auto constantLayout = renderState.pipelineKey.shader->GetConstantLayout();
        auto pipelineLayout = renderState.pipelineKey.shader->GetPipelineLayout()->layout;
        auto stageFlags = EnumConvert::GetShaderStageFlags(renderState.pipelineKey.shader->GetStageFlags());

        auto element = constantLayout.TryGetElement(nameHashId);

        if (element == nullptr || element->Size < size)
        {
            return;
        }

        if (element != nullptr)
        {
            vkCmdPushConstants(commandBuffer, pipelineLayout, element->StageFlags, element->Offset, size, data);
        }
    }

    void VulkanCommandBuffer::BeginRenderPass()
    {
        renderState.PrepareRenderPass();
        VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        renderPassInfo.renderPass = renderState.renderPass->renderPass;
        renderPassInfo.framebuffer = renderState.frameBuffer->frameBuffer;
        renderPassInfo.renderArea = renderState.renderArea;
        renderPassInfo.clearValueCount = renderState.clearValueCount;
        renderPassInfo.pClearValues = renderState.clearValues;
        BeginRenderPass(renderPassInfo);
    }

    void VulkanCommandBuffer::EndRenderPass()
    {
        VulkanRawCommandBuffer::EndRenderPass();
    }

    void VulkanCommandBuffer::ValidatePipeline()
    {
        auto flags = renderState.ValidatePipeline(GetOnCompleteGate());

        if ((flags & PK_RENDER_STATE_DIRTY_PIPELINE) != 0)
        {
            BindPipeline(EnumConvert::GetPipelineBindPoint(renderState.pipelineKey.shader->GetType()), renderState.pipeline->pipeline);
        }

        if ((flags & PK_RENDER_STATE_DIRTY_VERTEXBUFFERS) != 0)
        {
            SetVertexBuffers(0, renderState.vertexBufferBindCount, renderState.vertexBuffersRaw, renderState.vertexBufferOffsets);
        }

        for (auto i = 0; i < PK_MAX_DESCRIPTOR_SETS; ++i)
        {
            if ((flags & (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i)) != 0)
            {
                auto pipeline = renderState.pipeline;
                auto bindPoint = EnumConvert::GetPipelineBindPoint(renderState.pipelineKey.shader->GetType());
                BindDescriptorSets(bindPoint, renderState.pipelineKey.shader->GetPipelineLayout(), 0, 1, &renderState.descriptorSets[i], 0, nullptr);
            }
        }
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

    void VulkanCommandBuffer::DispatchCompute(const ShaderVariant* shader, uint3 groupCount)
    {
        auto vkshader = shader->GetNative<VulkanShader>();
        BindPipeline(EnumConvert::GetPipelineBindPoint(vkshader->GetType()), renderState.GetComputePipeline(vkshader)->pipeline);
        vkCmdDispatch(commandBuffer, groupCount.x, groupCount.y, groupCount.z);
    }

    void VulkanCommandBuffer::Blit(Texture* src, Window* dst, uint32_t dstLevel, uint32_t dstLayer, FilterMode filter) const
    {
        auto vksrc = src->GetNative<VulkanTexture>();
        auto vkwindow = dst->GetNative<VulkanWindow>();
        Blit(vksrc->GetRenderTarget(), vkwindow->GetRenderTarget(), 0, dstLevel, 0, dstLayer, filter);
    }

    void VulkanCommandBuffer::Blit(Texture* src, Texture* dst, uint32_t srcLevel, uint32_t dstLevel, uint32_t srcLayer, uint32_t dstLayer, FilterMode filter) const
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
                                   FilterMode filter) const
    {
        VkImageBlit blitRegion{};
        blitRegion.srcSubresource = { (uint32_t)src.aspect, srcLevel, srcLayer, 1 };
        blitRegion.dstSubresource = { (uint32_t)dst.aspect, dstLevel, dstLayer, 1 };
        blitRegion.srcOffsets[1] = { (int)src.extent.width, (int)src.extent.height, (int)src.extent.depth };
        blitRegion.dstOffsets[1] = { (int)src.extent.width, (int)src.extent.height, (int)src.extent.depth };

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

    void VulkanCommandBuffer::Barrier(const Texture* texture, const Buffer* buffer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags) const
    {
        VkImageMemoryBarrier imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        VkMemoryBarrier memoryBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        VkBufferMemoryBarrier bufferBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};

        if (texture != nullptr)
        {
            auto vktex = texture->GetNative<VulkanTexture>();
            auto vkrawtex = vktex->GetRaw();
            imageBarrier.oldLayout = imageBarrier.newLayout = vktex->GetImageLayout();
            imageBarrier.image = vkrawtex->image;
            imageBarrier.subresourceRange.aspectMask = vkrawtex->aspect;
            imageBarrier.subresourceRange.levelCount = vkrawtex->levels;
            imageBarrier.subresourceRange.layerCount = vkrawtex->layers;
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
}

