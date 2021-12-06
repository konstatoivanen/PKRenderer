#include "PrecompiledHeader.h"
#include "VulkanStructs.h"
#include "VulkanUtilities.h"

namespace PK::Rendering::VulkanRHI
{
    VulkanBufferCreateInfo::VulkanBufferCreateInfo(BufferUsage usage, size_t size)
    {
        buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer.size = size;

        switch (usage)
        {
            case BufferUsage::Vertex:
                buffer.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                allocation.usage = VMA_MEMORY_USAGE_GPU_ONLY;
                break;

            case BufferUsage::Index:
                buffer.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                allocation.usage = VMA_MEMORY_USAGE_GPU_ONLY;
                break;

            case BufferUsage::Staging:
                buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                allocation.usage = VMA_MEMORY_USAGE_CPU_ONLY;
                break;

            case BufferUsage::Uniform:
                buffer.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                allocation.usage = VMA_MEMORY_USAGE_GPU_ONLY;
                break;

            default: 
                PK_THROW_ERROR("Invalid buffer create preset!");
        }
    }

    VulkanImageCreateInfo::VulkanImageCreateInfo(const TextureDescriptor& descriptor)
    {
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = descriptor.samplerType == SamplerType::Sampler3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
        image.format = EnumConvert::GetFormat(descriptor.format);
        image.extent = { descriptor.resolution.x, descriptor.resolution.y, descriptor.resolution.z };
        image.mipLevels = descriptor.levels;
        image.arrayLayers = descriptor.layers;
        image.samples = EnumConvert::GetSampleCountFlags(descriptor.samples);
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = 0;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        allocation.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        aspect = (VkImageAspectFlagBits)0;

        if (descriptor.samplerType == SamplerType::Cubemap)
        {
            image.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        if (((uint32_t)descriptor.usage & (uint32_t)TextureUsage::Sample) != 0)
        {
            image.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        if (((uint32_t)descriptor.usage & (uint32_t)TextureUsage::RTColor) != 0)
        {
            image.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            if (((uint32_t)descriptor.usage & (uint32_t)TextureUsage::Sample) != 0)
            {
                image.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            }
        }

        if (((uint32_t)descriptor.usage & (uint32_t)TextureUsage::RTStencil) != 0)
        {
            aspect = VK_IMAGE_ASPECT_STENCIL_BIT;
            image.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }

        if (((uint32_t)descriptor.usage & (uint32_t)TextureUsage::Upload) != 0)
        {
            image.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if (((uint32_t)descriptor.usage & (uint32_t)TextureUsage::RTDepth) != 0)
        {
            aspect = (VkImageAspectFlagBits)((uint32_t)aspect | (uint32_t)VK_IMAGE_ASPECT_DEPTH_BIT);
            image.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if (aspect == (VkImageAspectFlagBits)0)
        {
            aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }

    VulkanLayoutTransition::VulkanLayoutTransition(VkImage target, VkImageLayout srcLayout, VkImageLayout dstLayout, const VkImageSubresourceRange& range)
    {
        image = target;
        oldLayout = srcLayout;
        newLayout = dstLayout;
        subresources = range;
        ApplyTransitionTemplate(newLayout);
    }

    VulkanLayoutTransition::VulkanLayoutTransition(VkImage target, VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageLayout impostorLayout, const VkImageSubresourceRange& range)
    {
        image = target;
        oldLayout = srcLayout;
        newLayout = dstLayout;
        subresources = range;
        ApplyTransitionTemplate(impostorLayout);
    }

    void VulkanLayoutTransition::ApplyTransitionTemplate(VkImageLayout layout)
    {
        switch (layout)
        {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                srcAccessMask = 0;
                dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                srcAccessMask = 0;
                dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_GENERAL:
                srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                dstAccessMask = 0;
                srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                srcAccessMask = 0;
                dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                break;

            default:
                PK_THROW_ERROR("Unsupported layout transition!");
        }
    }

    VulkanFence::VulkanFence(VkDevice device, bool signaled) : device(device)
    {
        VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

        if (signaled) 
        {
            fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }

        VK_ASSERT_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &vulkanFence));
    }
    
    VulkanFence::~VulkanFence()
    {
        vkDestroyFence(device, vulkanFence, nullptr);
    }
    
    VulkanSemaphore::VulkanSemaphore(VkDevice device) : device(device)
    {
        VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_ASSERT_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &vulkanSemaphore));
    }
    
    VulkanSemaphore::~VulkanSemaphore()
    {
        vkDestroySemaphore(device, vulkanSemaphore, nullptr);
    }

    VulkanImageView::VulkanImageView(VkDevice device, const VkImageViewCreateInfo& createInfo) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateImageView(device, &createInfo, nullptr, &view), "Failed to create an image view!");
    }

    VulkanImageView::~VulkanImageView()
    {
        vkDestroyImageView(device, view, nullptr);
    }
    
    VulkanFrameBuffer::VulkanFrameBuffer(VkDevice device, const VkFramebufferCreateInfo& createInfo) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateFramebuffer(device, &createInfo, nullptr, &frameBuffer), "Failed to create framebuffer!");
    }

    VulkanFrameBuffer::~VulkanFrameBuffer()
    {
        vkDestroyFramebuffer(device, frameBuffer, nullptr);
    }

    VulkanRenderPass::VulkanRenderPass(VkDevice device, const VkRenderPassCreateInfo& createInfo) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateRenderPass(device, &createInfo, nullptr, &renderPass), "Failed to create a render pass!");
    }

    VulkanRenderPass::~VulkanRenderPass()
    {
        vkDestroyRenderPass(device, renderPass, nullptr);
    }

    VulkanRawBuffer::VulkanRawBuffer(VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo) : allocator(allocator), usage(createInfo.buffer.usage), capacity(createInfo.buffer.size)
    {
        VK_ASSERT_RESULT_CTX(vmaCreateBuffer(allocator, &createInfo.buffer, &createInfo.allocation, &buffer, &memory, nullptr), "Failed to create a buffer!");
    }
    
    VulkanRawBuffer::~VulkanRawBuffer()
    {
        vmaDestroyBuffer(allocator, buffer, memory);
    }

    void* VulkanRawBuffer::BeginMap(size_t offset) const
    {
        void* mappedRange;
        vmaMapMemory(allocator, memory, &mappedRange);
        return reinterpret_cast<char*>(mappedRange) + offset;
    }

    void VulkanRawBuffer::EndMap(size_t offset, size_t size) const
    {
        vmaUnmapMemory(allocator, memory);
        vmaFlushAllocation(allocator, memory, offset, size);
    }

    void VulkanRawBuffer::SetData(const void* data, size_t size) const
    {
        memcpy(BeginMap(0ull), data, size);
        EndMap(0ull, size);
    }

    VulkanRawImage::VulkanRawImage(VmaAllocator allocator, const VulkanImageCreateInfo& createInfo) : 
        allocator(allocator),
        format(createInfo.image.format),
        type(createInfo.image.imageType),
        extent(createInfo.image.extent),
        levels(createInfo.image.mipLevels),
        layers(createInfo.image.arrayLayers),
        samples(createInfo.image.samples),
        aspect(createInfo.aspect)
    {
        VK_ASSERT_RESULT_CTX(vmaCreateImage(allocator, &createInfo.image, &createInfo.allocation, &image, &memory, nullptr), "Failed to create an image!");
    }

    VulkanRawImage::~VulkanRawImage()
    {
        vmaDestroyImage(allocator, image, memory);
    }

    VulkanShaderModule::VulkanShaderModule(VkDevice device, VkShaderStageFlagBits stage, const uint32_t* spirv, size_t sprivSize) : device(device)
    {
        VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        createInfo.codeSize = sprivSize;
        createInfo.pCode = spirv;

        VK_ASSERT_RESULT_CTX(vkCreateShaderModule(device, &createInfo, nullptr, &module), "Failed to create shader module!");

        stageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stageInfo.stage = stage;
        stageInfo.module = module;
        stageInfo.pName = "main";
    }

    VulkanShaderModule::~VulkanShaderModule()
    {
        vkDestroyShaderModule(device, module, nullptr);
    }

    VulkanPipeline::VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo& createInfo) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateGraphicsPipelines(device, pipelineCache, 1, &createInfo, nullptr, &pipeline), "failed to create a graphics pipeline!");
    }

    VulkanPipeline::~VulkanPipeline()
    {
        vkDestroyPipeline(device, pipeline, nullptr);
    }

    VulkanPipelineLayout::VulkanPipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& createInfo) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreatePipelineLayout(device, &createInfo, nullptr, &layout), "Failed to create a pipeline layout!");
    }

    VulkanPipelineLayout::~VulkanPipelineLayout()
    {
        vkDestroyPipelineLayout(device, layout, nullptr);
    }

    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo& createInfo) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout), "Failed to create a descriptor set layout!");
    }

    VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
    {
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }

    VulkanDescriptorPool::VulkanDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo& createInfo) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateDescriptorPool(device, &createInfo, nullptr, &pool), "Failed to create a descriptor pool");
    }

    VulkanDescriptorPool::~VulkanDescriptorPool()
    {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }

    VulkanSampler::VulkanSampler(VkDevice device, const SamplerDescriptor& descriptor) : device(device)
    {
        VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        info.addressModeU = EnumConvert::GetSamplerAddressMode(descriptor.wrap[0]);
        info.addressModeV = EnumConvert::GetSamplerAddressMode(descriptor.wrap[1]);
        info.addressModeW = EnumConvert::GetSamplerAddressMode(descriptor.wrap[2]);
        info.minLod = descriptor.mipMin;
        info.maxLod = descriptor.mipMax <= 0.0f ? VK_LOD_CLAMP_NONE : descriptor.mipMax;
        info.mipLodBias = descriptor.mipBias;
        info.maxAnisotropy = descriptor.anisotropy;
        info.anisotropyEnable = descriptor.anisotropy > 0.0f ? VK_TRUE : VK_FALSE;
        info.unnormalizedCoordinates = !descriptor.normalized;
        info.borderColor = EnumConvert::GetBorderColor(descriptor.borderColor);
        info.mipmapMode = (uint32_t)descriptor.filter > (uint32_t)FilterMode::Bilinear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.compareEnable = descriptor.comparison != Comparison::Off ? VK_TRUE : VK_FALSE;
        info.compareOp = EnumConvert::GetCompareOp(descriptor.comparison);
        info.magFilter = EnumConvert::GetFilterMode(descriptor.filter);
        info.minFilter = EnumConvert::GetFilterMode(descriptor.filter);
        VK_ASSERT_RESULT_CTX(vkCreateSampler(device, &info, nullptr, &sampler), "Failed to create a sampler!");
    }

    VulkanSampler::~VulkanSampler()
    {
        vkDestroySampler(device, sampler, nullptr);
    }

    void VulkanRawCommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo& beginInfo)
    {
        vkCmdBeginRenderPass(commandBuffer, &beginInfo, level == VK_COMMAND_BUFFER_LEVEL_PRIMARY ? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    }

    void VulkanRawCommandBuffer::EndRenderPass() const
    {
        vkCmdEndRenderPass(commandBuffer);
    }

    void VulkanRawCommandBuffer::BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) const
    {
        vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
    }

    void VulkanRawCommandBuffer::SetViewPorts(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) const
    {
        vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
    }

    void VulkanRawCommandBuffer::SetViewPort(const VkViewport& pViewport) const
    {
        SetViewPorts(0, 1, &pViewport);
    }

    void VulkanRawCommandBuffer::SetViewPort(const VkRect2D& rect, float minDepth, float maxDepth) const
    {
        VkViewport viewport{};
        viewport.x = (float)rect.offset.x;
        viewport.x = (float)rect.offset.y;
        viewport.width = (float)rect.extent.width;
        viewport.height = (float)rect.extent.height;
        viewport.minDepth = minDepth;
        viewport.maxDepth = maxDepth;
        SetViewPorts(0, 1, &viewport);
    }

    void VulkanRawCommandBuffer::SetScissors(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors) const
    {
        vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
    }

    void VulkanRawCommandBuffer::SetScissor(const VkRect2D& pScissor) const
    {
        SetScissors(0, 1, &pScissor);
    }

    void VulkanRawCommandBuffer::SetVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) const
    {
        vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
    }

    void VulkanRawCommandBuffer::SetVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const std::initializer_list<VkDeviceSize> pOffsets) const
    {
        vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets.begin());
    }

    void VulkanRawCommandBuffer::BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) const
    {
        vkCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
    }

    void VulkanRawCommandBuffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const
    {
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
    }

    void VulkanRawCommandBuffer::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) const
    {
        vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
    }

    void VulkanRawCommandBuffer::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, const VkExtent3D& extent, uint32_t level, uint32_t layer) const
    {
        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = level;
        region.imageSubresource.baseArrayLayer = layer;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = extent;
        CopyBufferToImage(srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }


    void VulkanRawCommandBuffer::PipelineBarrier(VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask,
        VkDependencyFlags dependencyFlags,
        uint32_t memoryBarrierCount,
        const VkMemoryBarrier* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount,
        const VkBufferMemoryBarrier* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount,
        const VkImageMemoryBarrier* pImageMemoryBarriers) const
    {
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

    void VulkanRawCommandBuffer::TransitionImageLayout(const VulkanLayoutTransition& transition) const
    {
        if (transition.oldLayout == transition.newLayout)
        {
            return;
        }

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = transition.oldLayout;
        barrier.newLayout = transition.newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = transition.image;
        barrier.subresourceRange = transition.subresources;
        barrier.srcAccessMask = transition.srcAccessMask;
        barrier.dstAccessMask = transition.dstAccessMask;
        PipelineBarrier(transition.srcStage, transition.dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void VulkanRawCommandBuffer::BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,
        const VulkanPipelineLayout* layout,
        uint32_t firstSet,
        uint32_t descriptorSetCount,
        const VulkanDescriptorSet** pDescriptorSets,
        uint32_t dynamicOffsetCount,
        const uint32_t* pDynamicOffsets) const
    {

        std::vector<VkDescriptorSet> sets;
        sets.resize(descriptorSetCount);

        for (auto i = 0u; i < descriptorSetCount; ++i)
        {
            sets.at(i) = pDescriptorSets[i]->set;
            pDescriptorSets[i]->executionGate = GetOnCompleteGate();
        }

        vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout->layout, firstSet, descriptorSetCount, sets.data(), dynamicOffsetCount, pDynamicOffsets);
    }
}