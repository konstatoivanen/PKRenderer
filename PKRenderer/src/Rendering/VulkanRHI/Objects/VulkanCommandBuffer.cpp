#include "PrecompiledHeader.h"
#include "VulkanCommandBuffer.h"
#include "Utilities/Handle.h"
#include "Rendering/VulkanRHI/Objects/VulkanBuffer.h"
#include "Rendering/VulkanRHI/Objects/VulkanTexture.h"
#include "Rendering/VulkanRHI/Objects/VulkanAccelerationStructure.h"
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
        renderState->SetRenderTarget(&handles, nullptr, 0u);
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
        renderState->SetIndexBuffer(handle, EnumConvert::GetIndexType(handle->buffer.layout->begin()->Type));
    }

    void VulkanCommandBuffer::SetBuffer(uint32_t nameHashId, Buffer* buffer, const IndexRange& range)
    {
        renderState->SetResource(nameHashId, Handle(buffer->GetNative<VulkanBuffer>()->GetBindHandle(range)));
    }

    void VulkanCommandBuffer::SetTexture(uint32_t nameHashId, Texture* texture, const TextureViewRange& range)
    {
        renderState->SetResource(nameHashId, Handle(texture->GetNative<VulkanTexture>()->GetBindHandle(range, TextureBindMode::SampledTexture)));
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
        renderState->SetResource(nameHashId, Handle(texture->GetNative<VulkanTexture>()->GetBindHandle(range, TextureBindMode::Image)));
    }

    void VulkanCommandBuffer::SetAccelerationStructure(uint32_t nameHashId, AccelerationStructure* structure)
    {
        renderState->SetResource(nameHashId, Handle(structure->GetNative<VulkanAccelerationStructure>()->GetBindHandle()));
    }

    void VulkanCommandBuffer::SetShaderBindingTable(Structs::RayTracingShaderGroup group, const Buffer* buffer, size_t offset, size_t stride, size_t size)
    {
        auto address = buffer->GetNative<VulkanBuffer>()->GetRaw()->deviceAddress;
        renderState->SetShaderBindingTableAddress(group, address + offset, stride, size);
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

    void VulkanCommandBuffer::DispatchRays(Math::uint3 dimensions)
    {
        EndRenderPass();
        ValidatePipeline();
        auto bundle = renderState->GetShaderBindingTableBundle();
        vkCmdTraceRaysKHR(commandBuffer, 
                          bundle.addresses + (uint32_t)Structs::RayTracingShaderGroup::RayGeneration, 
                          bundle.addresses + (uint32_t)Structs::RayTracingShaderGroup::Miss,
                          bundle.addresses + (uint32_t)Structs::RayTracingShaderGroup::Hit,
                          bundle.addresses + (uint32_t)Structs::RayTracingShaderGroup::Callable,
                          dimensions.x, 
                          dimensions.y, 
                          dimensions.z);
    }


    void VulkanCommandBuffer::Blit(Texture* src, Window* dst, FilterMode filter)
    {
        auto vksrc = src->GetNative<VulkanTexture>();
        auto vkwindow = dst->GetNative<VulkanWindow>();
        Blit(vksrc->GetBindHandle(TextureBindMode::RenderTarget), vkwindow->GetBindHandle(), 0, 0, 0, 0, filter, true);
    }

    void VulkanCommandBuffer::Blit(Window* src, Buffer* dst)
    {
        auto vksrc = src->GetNative<VulkanWindow>()->GetBindHandle();
        auto vkbuff = dst->GetNative<VulkanBuffer>();

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = vksrc->image.extent;

        VkImageSubresourceRange srcRange{};
        srcRange.aspectMask = vksrc->image.range.aspectMask;
        srcRange.baseMipLevel = 0;
        srcRange.levelCount = 1;
        srcRange.baseArrayLayer = 0;
        srcRange.layerCount = 1;

        TransitionImageLayout(VulkanLayoutTransition(vksrc->image.image, vksrc->image.layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcRange));
        vkCmdCopyImageToBuffer(commandBuffer, vksrc->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkbuff->GetRaw()->buffer, 1, &region);
        TransitionImageLayout(VulkanLayoutTransition(vksrc->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vksrc->image.layout, srcRange));
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

        VkImageSubresourceRange srcRange{};
        srcRange.aspectMask = src->image.range.aspectMask;
        srcRange.baseMipLevel = srcLevel;
        srcRange.levelCount = 1;
        srcRange.baseArrayLayer = srcLayer;
        srcRange.layerCount = src->image.range.layerCount;

        VkImageSubresourceRange dstRange{};
        dstRange.aspectMask = dst->image.range.aspectMask;
        dstRange.baseMipLevel = dstLevel;
        dstRange.levelCount = 1;
        dstRange.baseArrayLayer = dstLayer;
        dstRange.layerCount = dst->image.range.layerCount;

        TransitionImageLayout(VulkanLayoutTransition(src->image.image, src->image.layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcRange));
        TransitionImageLayout(VulkanLayoutTransition(dst->image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstRange));

        if (src->image.samples > 1 && dst->image.samples == 1)
        {
            vkCmdResolveImage(commandBuffer, src->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &resolveRegion);
        }
        else 
        {
            auto vkFilter = EnumConvert::GetFilterMode(filter);
            vkCmdBlitImage(commandBuffer, src->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, vkFilter);
        }

        TransitionImageLayout(VulkanLayoutTransition(src->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src->image.layout, srcRange));
        TransitionImageLayout(VulkanLayoutTransition(dst->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst->image.layout, dstRange));
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
        auto handle = vktex->GetBindHandle(range, TextureBindMode::Image);
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

        vkCmdClearColorImage(commandBuffer, vktex->GetRaw()->image, handle->image.layout, &clearValue, 1, &subrange);
    }


    void VulkanCommandBuffer::Barrier(const Texture* texture, const TextureViewRange& range, const Buffer* buffer, size_t offset, size_t size, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags)
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
            bufferBarrier.offset = offset;
            bufferBarrier.size = size;
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

    void* VulkanCommandBuffer::BeginBufferWrite(Buffer* buffer, size_t offset, size_t size)
    {
        return buffer->GetNative<VulkanBuffer>()->BeginWrite(offset, size);
    }

    void VulkanCommandBuffer::EndBufferWrite(Buffer* buffer)
    {
        VkBufferCopy copyRegion;
        VkBuffer srcBuffer;
        VkBuffer dstBuffer;

        auto vkBuffer = buffer->GetNative<VulkanBuffer>();
        vkBuffer->EndWrite(&srcBuffer, &dstBuffer, &copyRegion);

        CopyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

        VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = dstBuffer;
        barrier.offset = copyRegion.dstOffset;
        barrier.size = copyRegion.size;

        auto usage = vkBuffer->GetRaw()->usage;
        uint32_t dstStageMask = 0u;

        if (usage & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
        {
            barrier.dstAccessMask |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
            dstStageMask |= VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        }

        if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            barrier.dstAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
            dstStageMask |= VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        }

        if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
            barrier.dstAccessMask |= VK_ACCESS_MEMORY_READ_BIT;
            dstStageMask |= VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        }

        if (usage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
        {
            barrier.dstAccessMask |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            dstStageMask |= VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        }

        if (dstStageMask != 0)
        {
            PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, dstStageMask, 0, 0, nullptr, 1, &barrier, 0, nullptr);
        }
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

    void VulkanCommandBuffer::BuildAccelerationStructures(uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
    {
        vkCmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
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
            vkCmdBindIndexBuffer(commandBuffer, indexBufferHandle->buffer.buffer, indexBufferHandle->buffer.offset, indexType);
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
                    auto stageFlags = EnumConvert::GetShaderStageFlags(element.StageFlags);
                    vkCmdPushConstants(commandBuffer, renderState->GetPipelineLayout(), stageFlags, element.Offset, (uint32_t)dataSize, data);
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
